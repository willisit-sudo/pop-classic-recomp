#!/usr/bin/env python3
"""Build and launch the native test port using a local Xbox 360 game dump."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

KIT = Path(__file__).resolve().parent
SDK_VERSION = "0.11.0"
XEX_SHA256 = "6d212412482e40569d972ba1ed68fc3910412c396d8e0610d1722957516c520e"
IS_WINDOWS = os.name == "nt"


def game_executable(game_dir):
    files = {entry.name.casefold(): entry for entry in game_dir.iterdir() if entry.is_file()}
    required = ["default.xex", "info.bar", "logo_gameloft.wmv"]
    required.extend(f"data{index:03d}.bar" for index in range(9))
    missing = [name for name in required if name not in files]
    if missing:
        raise ValueError("Missing from the extracted game folder: " + ", ".join(missing))
    xex = files["default.xex"]
    with xex.open("rb") as stream:
        digest = hashlib.file_digest(stream, "sha256").hexdigest()
    if digest != XEX_SHA256:
        raise ValueError(f"Unsupported default.xex revision (SHA-256 {digest}). Expected {XEX_SHA256}.")
    return xex


def write_manifest(project_dir, game_dir, xex):
    def toml_path(path):
        return json.dumps(path.as_posix(), ensure_ascii=False)

    manifest = project_dir / "pop_classic_manifest.toml"
    text = f"""[project]
name = "pop_classic"
sdk_version = "{SDK_VERSION}"
game_root = {toml_path(game_dir)}
hook_sources = ["src"]

[entrypoint]
file_path = {toml_path(xex)}
out_directory_path = "generated/default"
includes = []
"""
    if not manifest.is_file() or manifest.read_text(encoding="utf-8") != text:
        manifest.write_text(text, encoding="utf-8")
    return manifest


def run(command, cwd, env):
    subprocess.run(command, cwd=cwd, env=env, check=True)


def build_and_play(args):
    game_dir = args.game_dir.resolve(strict=True)
    sdk_dir = args.sdk_dir.resolve(strict=True)
    xex = game_executable(game_dir)
    suffix = ".exe" if IS_WINDOWS else ""
    codegen = sdk_dir / "bin" / f"rexglue{suffix}"
    if not codegen.is_file():
        raise ValueError(f"ReXGlue CLI missing at {codegen}. Use the extracted SDK platform directory.")
    env = sdk_environment(sdk_dir)
    version_output = subprocess.check_output(
        [str(codegen), "--version"], env=env, text=True, stderr=subprocess.STDOUT
    )
    if version_output.strip().removeprefix("v") != SDK_VERSION:
        raise ValueError(f"Expected ReXGlue v{SDK_VERSION}. The CLI reported: {version_output.strip()}")

    work = args.work_dir.resolve()
    project = work / "project"
    build = work / "build"
    state = work / "state"
    (project / "src").mkdir(parents=True, exist_ok=True)
    state.mkdir(parents=True, exist_ok=True)
    for name in ("CMakeLists.txt", "src/main.cpp", "src/pop_classic_hooks.cpp"):
        source = KIT / name
        target = project / name
        if not target.is_file() or source.read_bytes() != target.read_bytes():
            shutil.copyfile(source, target)
    manifest = write_manifest(project, game_dir, xex)

    run([str(codegen), "codegen", str(manifest)], project, env)
    compiler = args.compiler or ("clang++" if IS_WINDOWS else "clang++-20")
    run(
        ["cmake", "-S", str(project), "-B", str(build), "-G", "Ninja",
         "-DCMAKE_BUILD_TYPE=Release", f"-DCMAKE_PREFIX_PATH={sdk_dir}",
         f"-DCMAKE_CXX_COMPILER={compiler}", *args.cmake_arg],
        project, env,
    )
    run(["cmake", "--build", str(build), "--parallel", str(args.jobs)], project, env)

    executable = build / f"pop_classic{suffix}"
    print(f"Local build ready: {executable}", flush=True)
    if not args.build_only:
        run(
            [str(executable), f"--game_data_root={game_dir}",
             f"--app.config_file={state / 'settings.toml'}",
             f"--user_data_root={state / 'profile'}", f"--cache_root={state / 'cache'}"],
            state, env,
        )


def sdk_environment(sdk_dir):
    env = os.environ.copy()
    name = "PATH" if IS_WINDOWS else "LD_LIBRARY_PATH"
    directories = [str(sdk_dir / "bin"), str(sdk_dir / "lib")] if IS_WINDOWS else [str(sdk_dir / "lib")]
    if env.get(name):
        directories.append(env[name])
    env[name] = os.pathsep.join(directories)
    return env


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game-dir", type=Path, required=True, help="extracted Xbox 360 game folder")
    parser.add_argument("--sdk-dir", type=Path, required=True, help="ReXGlue 0.11.0 platform SDK directory")
    parser.add_argument("--work-dir", type=Path, default=KIT / ".local", help="local generated files, build, and saves")
    parser.add_argument("--compiler", help="Clang C++ compiler path or executable name")
    parser.add_argument("--cmake-arg", action="append", default=[], help="extra configure option, repeatable")
    parser.add_argument("--jobs", type=int, default=min(os.cpu_count() or 1, 4), help="parallel compile jobs")
    parser.add_argument("--build-only", action="store_true", help="compile without opening the game")
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be at least 1")
    try:
        build_and_play(args)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"Setup failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

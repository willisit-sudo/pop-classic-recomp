# Prince of Persia Classic native test build

Build the Xbox 360 release locally from your own extracted game files. This kit contains the
host app, its Controls and Display panel, and a build script. The build script generates the
game-specific C++ and native executable locally.

## What you need

- The extracted Xbox 360 game folder containing `default.xex` and the accompanying assets.
- The [ReXGlue v0.11.0 SDK](https://github.com/rexglue/rexglue-sdk/releases/tag/v0.11.0)
  for your platform. Extract the SDK and use the folder containing `bin`, `include`, and `lib`.
- Python 3.11 or newer, CMake 3.25 or newer, Ninja, and Clang 20. On Linux x86-64,
  the SDK requires LLD and GCC 13's libstdc++ development files. If Clang uses a different
  GCC installation, pass
  `--cmake-arg=-DCMAKE_CXX_FLAGS=--gcc-install-dir=/path/to/gcc/13`.
- On Windows, use a developer terminal with the MSVC build tools and Windows SDK available
  to Clang. The Windows build has reached the title screen in local testing.

The supported XEX is title ID `58410865`, media ID `03EE2F9A`, version `0.0.1.4`.
The script validates the executable by SHA-256 before running code generation.

See [platform notes](PLATFORM-NOTES.md) for verified behavior and open test areas.

## Build and play

On Linux, from the extracted kit folder:

```sh
python3 build.py --game-dir "/path/to/extracted game" --sdk-dir "/path/to/linux-amd64"
```

On Windows, from a developer PowerShell:

```powershell
py -3 build.py --game-dir "C:\Games\PoP Classic Extracted" --sdk-dir "C:\SDKs\win-amd64"
```

The first build can take several minutes. Re-run the same command to launch again. The build
is incremental. Add `--build-only` to compile without opening the game, or use `--help` to
see compiler and build-directory options. Press **F2** in-game to open Controls and Display.

The source kit uses the runtime's default license handling. It applies no local license-mask
override. Generated sources, executables, game files, caches, and user data are local to the
player. The default location for generated sources, the build, and player data is `.local/`,
which Git ignores. Keep that directory and any personal game dump out of shared archives.

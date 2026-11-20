# Prince of Persia Classic for Windows and Linux

**Download a prebuilt player and run it. No compiler or build tools needed.** You’ll need your own extracted Xbox 360 copy of Prince of Persia Classic, with `default.xex` and the accompanying game files in the same folder.

| Platform | Download | Run after extracting |
| --- | --- | --- |
| Windows x64 | [Windows Player ZIP](https://github.com/willisit-sudo/pop-classic-recomp/releases/download/v0.1.0-preview/PoP-Classic-Windows-Player.zip) | `Play Prince of Persia Classic.exe` |
| Linux x64 / Steam Deck | [Linux Player tar.gz](https://github.com/willisit-sudo/pop-classic-recomp/releases/download/v0.1.0-preview/PoP-Classic-Linux-Player.tar.gz) | `Play.sh` |

On first launch, select the folder containing `default.xex`. The launcher remembers it for next time. Press **F2** in-game for Controls and Display. For built-in Deck controls, add `Play.sh` as a non-Steam game and select Steam’s **Gamepad** layout.

These are preview builds. See the [release notes](https://github.com/willisit-sudo/pop-classic-recomp/releases/tag/v0.1.0-preview) and [platform notes](PLATFORM-NOTES.md) for testing details.

## Build from source (optional)

The repository contains the host app, its Controls and Display panel, and a build script that generates the native executable from your extracted game files.

### Build prerequisites


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

### Build and run

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

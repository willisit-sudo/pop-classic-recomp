# Platform notes

## Linux and Steam Deck

The native Release build was smoke-tested on the Deck with ReXGlue v0.11.0 and Vulkan. A clean build loaded the supported XEX and the game submitted frames. The F2 panel opened at 800×450. Keyboard play and the Deck’s built-in controller were also tested manually. To get the Deck controls, add your locally built launcher as a non-Steam game and choose Steam’s Gamepad layout. The Deck controls worked in Desktop Mode when launched through Steam.

The build script keeps generated sources, settings, saves, and caches in its local working directory. You can pass `--work-dir` to choose another location. The private player prototype also tested remembering a selected folder across launches.

## Windows x64

The native Release build succeeded with ReXGlue v0.11.0, Clang 20.1.8, and the Visual Studio 2022 build tools. The user confirmed that the private player reached the title screen. Logs showed Direct3D 12 initialization, shader cache creation, and a connected controller. The exported Windows player was already compiled. Running its launcher `.exe` opened a folder picker for the extracted game files, and the launcher remembered the selection. The export required no compiler or build toolchain on the player’s PC.

When building from this repository, if the Python launcher cannot find your Python install, invoke the installed `python.exe` directly. Build outside synchronized folders if dependency files become locked during the build. Test the available windowed and fullscreen options in the F2 panel.

## Test coverage

Full playthroughs, save and reload behavior, and clean shutdown still need wider testing. The public kit uses ReXGlue’s default license handling, and the title may show Trial depending on the runtime license state. Build with your own extracted copy.

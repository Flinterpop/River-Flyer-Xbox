# River Flyer Xbox

*Last updated: 17 Sep 2026*

A from-scratch engine for taking [River Flyer](../River-Flyer) to an Xbox as a native app: no raylib, no third-party code, only what Windows and the console provide. It is a learning project first. The engine is small on purpose, each module maps onto one thing Microsoft's platform gives you, and the existing game compiles against it unchanged through a compatibility header, so the desktop raylib build stays the reference.

## Why an engine at all

There is no OpenGL on Xbox, so raylib's renderer cannot go there. River Flyer calls 86 of raylib's 600 functions, and those 86 are the whole surface to rebuild: a window and frame loop, a Direct3D 11 device, a 2D sprite batch with shapes and text, procedural images, gamepad and keyboard input, an XAudio2 mixer, a writable save folder, timing and random numbers. Everything above that line, the terrain, pilots, missiles and scoring, is engine-neutral already.

## Layout

| Path | Holds |
|---|---|
| `engine/include/rf/*.h` | The engine API, one header per subsystem: `Platform`, `Graphics`, `Draw`, `Text`, `Input`, `Audio`, `Storage`, `Random`, `Log`, `Types` |
| `engine/src/d3d11/` | Direct3D 11: device and flip-model swap chain, textures with mipmaps, render targets, the sprite batch, `Sprite.hlsl` (compiled by fxc at build time) |
| `engine/src/text/` | The 5 x 7 pixel font drawn in code, rasterised into a glyph atlas at start-up |
| `engine/src/xaudio2/` | XAudio2: one mastering voice, one source voice per synthesised sound |
| `engine/src/input/`, `engine/src/storage/` | Platform-neutral halves: keyboard state, queues and queries; file read and write in the save folder |
| `engine/src/win32/` | PC backends: `CreateWindowExW` and the message pump, XInput, `%LOCALAPPDATA%` |
| `engine/src/uwp/` | Developer Mode Xbox backends: `CoreWindow` shell and lifecycle, `Windows.Gaming.Input`, `LocalFolder` |
| `compat/raylib.h` | The 86 raylib declarations the game uses, with raylib's exact signatures and constants |
| `compat/raylib_compat.cpp` | Those 86 implemented on the engine, plus the CPU image drawing and Perlin noise the sprite generator needs |
| `demo/main.cpp` | Phase 2: window, controller, letterboxed canvas, a plane that follows the stick |
| `uwp/` | The console package: manifest, tile art, build and deploy steps |

Notes:

- Same rules as the game: no heap allocation after start-up (tables are `std::array`, handles are indices; the sound buffers and images are allocated while the game generates them at start-up), every loop bounded, assertions on inputs and results, `/W4 /WX`.
- `rf::input` numbers keys and buttons exactly as raylib does, so the compat layer is a cast. That is a convenience, not a dependency.
- The Windows headers declare a GDI function called `Rectangle`, which collides with the raylib-shaped struct. Every target compiles with `NOGDI`; the one file that needs a GDI type (XAudio2's `mmreg.h`) undefines it locally.

## Build and run

Requirements: Visual Studio 2026 with the Desktop C++ workload, the Universal Windows Platform workload for the console build, and a Windows 10 SDK (Direct3D 11, XAudio2, XInput, fxc and the C++/WinRT headers ship with it). No vcpkg, no downloads.

```powershell
cmake --preset msvc
cmake --build --preset release        # or: debug (console window with the log, Direct3D debug layer)
build\Release\riverflyer.exe          # the game on the engine
build\Release\demo.exe                # the engine alone: stick or WASD moves the plane, Space or A fires, Esc quits

cmake --preset uwp
cmake --build --preset uwp-release    # the console package; see uwp\README.md to deploy it
```

`riverflyer` is built only when `..\River-Flyer\src` exists (override with `-DRF_GAME_DIR=`). The game's saves live in `%LOCALAPPDATA%\RiverFlyer\` on the PC, separate from the raylib build's files beside its exe.

## Plan

Each phase ends with something visible.

1. [ ] **Pipeline.** Enable Developer Mode on the console and deploy Visual Studio's DirectX 11 UWP template unchanged. A cube on the TV proves the tools, the network deploy and the account before any of this project is involved. Needs the console; see `uwp/README.md`.
2. [x] **Shell, device, input.** `demo.exe`: window, clear, controller and keyboard, a render target scaled to the window.
3. [x] **Text.** The pixel-font glyph atlas. Scaled non-uniformly (about 0.4 em advance, 0.7 em tall) to match the proportions of raylib's default face, so the game's panels lay out as designed. `ImageDrawText` blits the same glyphs into sprites.
4. [x] **Audio.** XAudio2, with the game's own synthesised 16-bit mono clips.
5. [x] **The game.** `riverflyer.exe` runs on the engine with terrain, sprites, text and sound. Known differences are listed below.
6. [x] **Console build.** The `uwp` preset builds a signed, sideloadable package with the game's manifest and tile art; suspend calls `IDXGIDevice3::Trim`, resume resets the clock; shaders are precompiled. Untested on a console until phase 1 is done.
7. [ ] **On the TV.** Deploy through the Device Portal, then check the overscan band, controller disconnects, and how the 4:5 canvas sits on a 16:9 screen.
8. [ ] **Later, if wanted:** the GDK. The public PC GDK swaps XInput for GameInput and the storage folder for XGameSave; the console half comes with ID@Xbox acceptance.

## Deliberate differences from raylib

All listed in `compat/raylib.h` beside the declaration. The ones that matter:

- `GetApplicationDirectory()` returns the writable folder, not the exe's. A console cannot write beside the exe, and the game's save path is built from this call.
- `SetTargetFPS()` does nothing. Presentation is vsync-paced, so the frame rate is the display's; the game is time-based and does not care.
- Render targets are not upside down. The game flips them with a negative source height for OpenGL; `DrawTexturePro` undoes that for target textures so the same call works on both.
- `GenImagePerlinNoise` is classic gradient noise with six octaves, not stb_perlin. Same character, different pixels.
- The font is a pixel face, not raylib's. Same metrics, different letterforms.
- `Camera2D` rotation is ignored (the game never rotates). `LoadTexture` from a file is not implemented (the game has no files).

## Two things the debug layer taught

- raylib-style code draws into a render target before `BeginDrawing`, which raylib tolerates because its state is global. Direct3D draws nothing without a bound shader and topology, and with the debug layer on it removes the device. So the pipeline is bound at device creation, not per frame.
- When the device is lost, `Graphics_D3D11.cpp` prints the debug layer's stored messages. Build Debug with the Graphics Tools feature installed and the reason is usually in the first line.

## Where to read

Microsoft Learn is the primary source for all of it: the Direct3D 11 programming guide (start with "Getting started with Direct3D 11" and the DXGI flip model), "XAudio2 programming guide", XInput, "Windows.Gaming.Input" and "CoreApplication" for UWP, and the Xbox Developer Mode pages. The DirectXTK source on GitHub is a good second read for how a production sprite batch and audio engine are structured, even though this project uses neither.

## License

[MIT](LICENSE), the same as River Flyer. The engine contains no third-party code; the pixel font is drawn here.

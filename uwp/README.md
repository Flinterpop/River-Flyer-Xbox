# Developer Mode Xbox build (UWP)

*Last updated: 17 Sep 2026*

This folder holds what the console build adds on top of the engine: the app manifest and the tile art. The code that differs lives in `engine/src/uwp/` and is selected by the `uwp` CMake preset. Everything else, Direct3D 11, the sprite batch, the pixel font, XAudio2 and the raylib compatibility layer, is the same source on the PC and on the console.

## What the console backend replaces

| Engine module | PC (`engine/src/win32/`) | Xbox (`engine/src/uwp/`) |
|---|---|---|
| `Platform` | `CreateWindowExW` and a `PeekMessage` loop | `CoreApplication::Run` with an `IFrameworkView`; `Run()` calls the game's own `main()`, and `PumpEvents()` drains the `CoreWindow` dispatcher |
| Swap chain | `CreateSwapChainForHwnd` | `CreateSwapChainForCoreWindow`; one `#if` in `Graphics_D3D11.cpp`, nothing else |
| Keyboard | `WM_KEYDOWN`, `WM_CHAR` | `CoreWindow::KeyDown`, `CharacterReceived`; same key numbering |
| Gamepads | XInput | `Windows::Gaming::Input::Gamepad` readings; stick Y is +1 up, so negated |
| Save folder | `%LOCALAPPDATA%\RiverFlyer\` | `ApplicationData::Current().LocalFolder()` |
| Lifecycle | none | `Suspending` calls `gfx::Trim()`, `Resuming` resets the frame clock |
| Entry point | `main()` | `WinMain` in `Platform_Uwp.cpp`, which starts `CoreApplication` and ends up in the same `main()` |

Notes:

- The console suspends the app behind the Xbox button and terminates apps that keep graphics memory through it, so the `Suspending` handler is not optional. The two save records are written by the game as they change, so nothing else needs saving there.
- The console assumes TV overscan. The game draws into its own 960 x 1000 canvas, which the letterbox path keeps inside the screen, but HUD text at the canvas edge may still sit in the overscan band on some TVs. `ApplicationView::VisibleBounds` gives the safe area if that turns out to matter.
- Developer Mode apps get a slice of the console, not all of it. This game needs far less than the slice.

## Build

```powershell
cmake --preset uwp
cmake --build --preset uwp-release
```

The `uwp` preset configures CMake for `WindowsStore` 10.0, which makes Visual Studio's app-packaging targets run: the build signs the package with a temporary `CN=CMake` certificate it generates in `build-uwp\riverflyer.dir\` and writes a sideloadable package to `build-uwp\AppPackages\riverflyer\riverflyer_<version>_x64_<config>_Test\`. That folder holds the `.msix`, the `.cer` for the certificate (also in the earlier `_Test` folders, it is the same key), and a `Dependencies` folder with the Visual C++ runtime package the console needs.

The manifest is `Package.appxmanifest` here. Its `Publisher` must match the certificate, so leave `CN=CMake` while sideloading; a store submission would use a real identity and certificate. The version in it is bumped by hand.

## Deploy to the console

1. On the console, install the **Xbox Dev Mode Activation** app from the store and follow it. It needs a Partner Center developer account (a one-time fee, historically about 20 US dollars). Check the current fee and terms on Microsoft's Xbox developer pages first: Microsoft has been moving Xbox development from UWP to the GDK for years, so confirm UWP sideloading is still supported before paying.
2. Switch the console to Developer Mode. Dev Home shows its address on the network and the Device Portal login.
3. Open the Device Portal in a browser on the PC, go to **My games and apps** and add the `.msix` from the `_Test` folder. When it asks for dependencies, add the `x64` package from `Dependencies`. When it asks for the certificate, add the `.cer`.
4. The tile appears in Dev Home. Play it from there.

Alternatively, Visual Studio can deploy and debug over the network: open `build-uwp\RiverFlyerXbox.slnx`, set `riverflyer` as the start-up project, choose **Remote Machine** with the console's address, and press F5. That is the route for stepping through code on the console.

## First milestone

Before deploying this project, deploy Visual Studio's own **DirectX 11 App (Universal Windows)** template unchanged. A spinning cube on the TV proves the account, the tools and the network path before anything from here is involved.

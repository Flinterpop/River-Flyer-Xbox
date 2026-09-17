// The Developer Mode Xbox shell: rf::platform on a CoreWindow (UWP).
//
// UWP inverts the control flow: the framework owns the window and calls
// IFrameworkView::Run on the window's thread. Run() simply calls the game's
// own main(), so main.cpp is the same file on the PC and on the console;
// platform::Init() then finds the CoreWindow already made for it, and
// PumpEvents() drains its dispatcher instead of a Win32 message loop.
#include "rf/Platform.h"

#include <cassert>

#include <windows.h>
#include <unknwn.h>
#include <winrt/base.h>
#include <winrt/Windows.ApplicationModel.Activation.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Display.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.Core.h>

#include "rf/Graphics.h"
#include "rf/Input.h"
#include "rf/Log.h"

int main();   // the game's entry point; runs on the CoreWindow thread from View::Run

namespace rf::platform {

namespace {

using winrt::Windows::ApplicationModel::SuspendingEventArgs;
using winrt::Windows::ApplicationModel::Activation::IActivatedEventArgs;
using winrt::Windows::ApplicationModel::Core::CoreApplication;
using winrt::Windows::ApplicationModel::Core::CoreApplicationView;
using winrt::Windows::ApplicationModel::Core::IFrameworkView;
using winrt::Windows::ApplicationModel::Core::IFrameworkViewSource;
using winrt::Windows::Foundation::IInspectable;
using winrt::Windows::Graphics::Display::DisplayInformation;
using winrt::Windows::UI::Core::CharacterReceivedEventArgs;
using winrt::Windows::UI::Core::CoreProcessEventsOption;
using winrt::Windows::UI::Core::CoreWindow;
using winrt::Windows::UI::Core::CoreWindowEventArgs;
using winrt::Windows::UI::Core::KeyEventArgs;
using winrt::Windows::UI::Core::WindowSizeChangedEventArgs;

constexpr float kMaxFrameSeconds = 0.25f;

struct State {
    CoreWindow    window {nullptr};
    int           width {0};
    int           height {0};
    float         dpiScale {1.0f};   // CoreWindow sizes are in DIPs; the swap chain wants pixels
    bool          closeRequested {false};
    LARGE_INTEGER freq {};
    LARGE_INTEGER start {};
    LARGE_INTEGER last {};
    float         frameSeconds {0.0f};
};

State g;

int Pixels(float dips)
{
    return static_cast<int>(dips * g.dpiScale + 0.5f);
}

// Windows::System::VirtualKey values are the Win32 VK codes.
input::Key KeyFromVk(int vk, bool extended)
{
    using input::Key;
    switch (vk) {
        case VK_SHIFT: case VK_LSHIFT:   return Key::LeftShift;
        case VK_RSHIFT:                  return Key::RightShift;
        case VK_CONTROL:                 return extended ? Key::RightControl : Key::LeftControl;
        case VK_LCONTROL:                return Key::LeftControl;
        case VK_RCONTROL:                return Key::RightControl;
        case VK_MENU:                    return extended ? Key::RightAlt : Key::LeftAlt;
        case VK_SPACE:      return Key::Space;
        case VK_ESCAPE:     return Key::Escape;
        case VK_RETURN:     return Key::Enter;
        case VK_TAB:        return Key::Tab;
        case VK_BACK:       return Key::Backspace;
        case VK_LEFT:       return Key::Left;
        case VK_RIGHT:      return Key::Right;
        case VK_UP:         return Key::Up;
        case VK_DOWN:       return Key::Down;
        case VK_OEM_7:      return Key::Apostrophe;
        case VK_OEM_COMMA:  return Key::Comma;
        case VK_OEM_MINUS:  return Key::Minus;
        case VK_OEM_PERIOD: return Key::Period;
        case VK_OEM_2:      return Key::Slash;
        case VK_OEM_1:      return Key::Semicolon;
        case VK_OEM_PLUS:   return Key::Equal;
        default:            break;
    }
    if (vk >= '0' && vk <= '9') { return static_cast<Key>(vk); }
    if (vk >= 'A' && vk <= 'Z') { return static_cast<Key>(vk); }
    if (vk >= VK_F1 && vk <= VK_F12) { return static_cast<Key>(static_cast<int>(Key::F1) + (vk - VK_F1)); }
    return Key::Null;
}

void OnKey(KeyEventArgs const& e, bool down)
{
    const auto status = e.KeyStatus();
    input::detail::OnKey(KeyFromVk(static_cast<int>(e.VirtualKey()), status.IsExtendedKey), down, down && status.WasKeyDown);
}

struct View : winrt::implements<View, IFrameworkViewSource, IFrameworkView> {
    IFrameworkView CreateView() { return *this; }

    void Initialize(CoreApplicationView const& view)
    {
        view.Activated([](CoreApplicationView const&, IActivatedEventArgs const&) { CoreWindow::GetForCurrentThread().Activate(); });
        // The console suspends the app behind the Xbox button and terminates
        // apps that keep graphics memory through it.
        CoreApplication::Suspending([](IInspectable const&, SuspendingEventArgs const& args) {
            auto deferral = args.SuspendingOperation().GetDeferral();
            gfx::Trim();
            deferral.Complete();
        });
        CoreApplication::Resuming([](IInspectable const&, IInspectable const&) { ResetClock(); });
    }

    void SetWindow(CoreWindow const& window)
    {
        g.window = window;
        window.Closed([](CoreWindow const&, CoreWindowEventArgs const&) { g.closeRequested = true; });
        window.SizeChanged([](CoreWindow const&, WindowSizeChangedEventArgs const& args) {
            g.width  = Pixels(args.Size().Width);
            g.height = Pixels(args.Size().Height);
        });
        window.KeyDown([](CoreWindow const&, KeyEventArgs const& e) { OnKey(e, true); });
        window.KeyUp([](CoreWindow const&, KeyEventArgs const& e) { OnKey(e, false); });
        window.CharacterReceived([](CoreWindow const&, CharacterReceivedEventArgs const& e) {
            const unsigned int c = e.KeyCode();
            if (c >= 32 && c < 0xD800) { input::detail::OnChar(static_cast<int>(c)); }   // printable BMP only
        });
    }

    void Load(winrt::hstring const&) {}
    void Run() { (void)main(); }   // returning ends the app
    void Uninitialize() {}
};

} // namespace

bool Init(const Config& cfg)
{
    assert(g.window != nullptr && cfg.title != nullptr);
    (void)cfg;   // the console picks the size: the window is the whole screen
    g.dpiScale = DisplayInformation::GetForCurrentView().LogicalDpi() / 96.0f;
    const auto bounds = g.window.Bounds();
    g.width  = Pixels(bounds.Width);
    g.height = Pixels(bounds.Height);
    g.window.Activate();

    (void)QueryPerformanceFrequency(&g.freq);
    (void)QueryPerformanceCounter(&g.start);
    g.last = g.start;
    g.frameSeconds = 0.0f;
    assert(g.width > 0 && g.height > 0);
    log::Write(log::Level::Info, "PLATFORM: CoreWindow %d x %d (scale %.2f)", g.width, g.height, g.dpiScale);
    return true;
}

void Shutdown()
{
}

void PumpEvents()
{
    assert(g.window != nullptr);
    input::detail::NewFrame();
    g.window.Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
    input::detail::PollPads();

    LARGE_INTEGER now {};
    (void)QueryPerformanceCounter(&now);
    const double dt = static_cast<double>(now.QuadPart - g.last.QuadPart) / static_cast<double>(g.freq.QuadPart);
    g.last = now;
    g.frameSeconds = (dt > kMaxFrameSeconds) ? kMaxFrameSeconds : static_cast<float>(dt);
}

void ResetClock()
{
    (void)QueryPerformanceCounter(&g.last);
    g.frameSeconds = 0.0f;
}

bool   CloseRequested() { return g.closeRequested; }
void   RequestClose()   { g.closeRequested = true; }
int    Width()          { return g.width; }
int    Height()         { return g.height; }
void*  NativeWindow()   { return winrt::get_unknown(g.window); }   // IUnknown* of the CoreWindow, for CreateSwapChainForCoreWindow
float  FrameSeconds()   { return g.frameSeconds; }

double Seconds()
{
    LARGE_INTEGER now {};
    (void)QueryPerformanceCounter(&now);
    return static_cast<double>(now.QuadPart - g.start.QuadPart) / static_cast<double>(g.freq.QuadPart);
}

} // namespace rf::platform

int __stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR, int)   // the CRT entry CMake's WIN32_EXECUTABLE expects
{
    winrt::init_apartment(winrt::apartment_type::multi_threaded);
    rf::platform::CoreApplication::Run(winrt::make<rf::platform::View>());
    return 0;
}

#include "rf/Platform.h"

#include <cassert>

#include <windows.h>

#include "rf/Input.h"
#include "rf/Log.h"

namespace rf::platform {

namespace {

constexpr wchar_t kClassName[]     = L"RiverFlyerWindow";
constexpr int     kMaxMessagesPump = 1024;   // per PumpEvents: bounds the message loop
constexpr float   kMaxFrameSeconds = 0.25f;

struct State {
    HWND          hwnd {nullptr};
    int           width {0};
    int           height {0};
    bool          closeRequested {false};
    LARGE_INTEGER freq {};
    LARGE_INTEGER start {};
    LARGE_INTEGER last {};
    float         frameSeconds {0.0f};
};

State g;

// Virtual key + lParam -> engine key. Left/right modifiers need the scan code
// or the extended bit because Windows reports both sides as one VK.
input::Key KeyFromVk(WPARAM vk, LPARAM lp)
{
    using input::Key;
    const UINT scan     = static_cast<UINT>((lp >> 16) & 0xFF);
    const bool extended = (lp & (1 << 24)) != 0;
    switch (vk) {
        case VK_SHIFT:      return (MapVirtualKeyW(scan, MAPVK_VSC_TO_VK_EX) == VK_RSHIFT) ? Key::RightShift : Key::LeftShift;
        case VK_CONTROL:    return extended ? Key::RightControl : Key::LeftControl;
        case VK_MENU:       return extended ? Key::RightAlt : Key::LeftAlt;
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
    if (vk >= '0' && vk <= '9') { return static_cast<Key>(vk); }           // same numbering
    if (vk >= 'A' && vk <= 'Z') { return static_cast<Key>(vk); }
    if (vk >= VK_F1 && vk <= VK_F12) { return static_cast<Key>(static_cast<int>(Key::F1) + static_cast<int>(vk - VK_F1)); }
    return Key::Null;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
        case WM_CLOSE:
            g.closeRequested = true;
            return 0;
        case WM_SIZE:
            if (wp != SIZE_MINIMIZED && LOWORD(lp) > 0 && HIWORD(lp) > 0) {
                g.width  = LOWORD(lp);
                g.height = HIWORD(lp);
            }
            return 0;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            input::detail::OnKey(KeyFromVk(wp, lp), true, (lp & (1 << 30)) != 0);
            break;   // fall through to DefWindowProc so Alt+F4 and friends keep working
        case WM_KEYUP:
        case WM_SYSKEYUP:
            input::detail::OnKey(KeyFromVk(wp, lp), false, false);
            break;
        case WM_CHAR:
            if (wp >= 32 && wp < 0xD800) { input::detail::OnChar(static_cast<int>(wp)); }   // printable BMP only
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void DrainMessages()
{
    MSG msg {};
    for (int i = 0; i < kMaxMessagesPump && PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE); ++i) {
        if (msg.message == WM_QUIT) { g.closeRequested = true; }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

} // namespace

bool Init(const Config& cfg)
{
    assert(g.hwnd == nullptr);
    assert(cfg.width > 0 && cfg.height > 0 && cfg.title != nullptr);
    (void)SetProcessDPIAware();   // client size in real pixels on a scaled desktop

    const HINSTANCE inst = GetModuleHandleW(nullptr);
    WNDCLASSEXW wc {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = inst;
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kClassName;
    if (RegisterClassExW(&wc) == 0) {
        log::Write(log::Level::Error, "PLATFORM: RegisterClassEx failed (%lu)", GetLastError());
        return false;
    }

    const DWORD style = cfg.resizable ? WS_OVERLAPPEDWINDOW : (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);
    RECT r {0, 0, cfg.width, cfg.height};
    (void)AdjustWindowRect(&r, style, FALSE);

    wchar_t title[128] = {0};
    for (int i = 0; i < 127 && cfg.title[i] != '\0'; ++i) { title[i] = static_cast<wchar_t>(cfg.title[i]); }   // ASCII titles

    g.hwnd = CreateWindowExW(0, kClassName, title, style, CW_USEDEFAULT, CW_USEDEFAULT,
                             r.right - r.left, r.bottom - r.top, nullptr, nullptr, inst, nullptr);
    if (g.hwnd == nullptr) {
        log::Write(log::Level::Error, "PLATFORM: CreateWindowEx failed (%lu)", GetLastError());
        return false;
    }
    g.width  = cfg.width;
    g.height = cfg.height;
    ShowWindow(g.hwnd, SW_SHOW);
    DrainMessages();   // WM_SIZE and friends from ShowWindow, so Width()/Height() are final

    (void)QueryPerformanceFrequency(&g.freq);
    (void)QueryPerformanceCounter(&g.start);
    g.last = g.start;
    g.frameSeconds = 0.0f;
    assert(g.freq.QuadPart > 0 && g.width > 0 && g.height > 0);
    log::Write(log::Level::Info, "PLATFORM: window %d x %d", g.width, g.height);
    return true;
}

void Shutdown()
{
    if (g.hwnd != nullptr) {
        DestroyWindow(g.hwnd);
        g.hwnd = nullptr;
    }
    UnregisterClassW(kClassName, GetModuleHandleW(nullptr));
}

void PumpEvents()
{
    assert(g.hwnd != nullptr);
    input::detail::NewFrame();
    DrainMessages();
    input::detail::PollPads();

    LARGE_INTEGER now {};
    (void)QueryPerformanceCounter(&now);
    const double dt = static_cast<double>(now.QuadPart - g.last.QuadPart) / static_cast<double>(g.freq.QuadPart);
    g.last = now;
    g.frameSeconds = (dt > kMaxFrameSeconds) ? kMaxFrameSeconds : static_cast<float>(dt);
    assert(g.frameSeconds >= 0.0f && g.frameSeconds <= kMaxFrameSeconds);
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
void*  NativeWindow()   { return g.hwnd; }
float  FrameSeconds()   { return g.frameSeconds; }

double Seconds()
{
    LARGE_INTEGER now {};
    (void)QueryPerformanceCounter(&now);
    return static_cast<double>(now.QuadPart - g.start.QuadPart) / static_cast<double>(g.freq.QuadPart);
}

} // namespace rf::platform

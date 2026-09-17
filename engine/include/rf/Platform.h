#pragma once

// The app shell: one window, the OS message pump, and the clock.
//
// Backends: engine/src/win32/Platform_Win32.cpp (PC, and the GDK console shell
// later, which is also a Win32-style window) and uwp/ for a Developer Mode
// Xbox, where CoreWindow replaces HWND. The game never sees either.
namespace rf::platform {

struct Config {
    int         width;       // client area in pixels
    int         height;
    const char* title;       // ASCII
    bool        resizable;   // false: fixed-size window (the desktop game); true: TV / browser style
};

bool   Init(const Config& cfg);
void   Shutdown();

// Once per frame, before reading input: rolls the input state, processes OS
// messages, polls the controllers and advances the frame clock.
void   PumpEvents();

bool   CloseRequested();     // user closed the window, or RequestClose() was called
void   RequestClose();

int    Width();              // current client size; follows resizes
int    Height();

// The OS window as an opaque pointer (HWND on Win32). Only the graphics
// backend dereferences it, so this header stays free of <windows.h>.
void*  NativeWindow();

double Seconds();            // monotonic seconds since Init
float  FrameSeconds();       // duration of the previous frame, capped at 0.25 s
void   ResetClock();         // after a console resume: the next frame's dt starts from now

} // namespace rf::platform

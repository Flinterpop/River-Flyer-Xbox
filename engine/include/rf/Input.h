#pragma once

// Keyboard and gamepads. State is sampled once per frame by
// platform::PumpEvents(); "pressed" means down this frame and up the last.
//
// Key and button numbering follows raylib (which follows GLFW) so the compat
// layer is a cast. Backends: engine/src/win32/Input_Win32.cpp (window
// messages + XInput); Windows.Gaming.Input under UWP; GameInput under the GDK.
namespace rf::input {

enum class Key : int {
    Null = 0,
    Space = 32, Apostrophe = 39, Comma = 44, Minus = 45, Period = 46, Slash = 47,
    Zero = 48, One, Two, Three, Four, Five, Six, Seven, Eight, Nine,
    Semicolon = 59, Equal = 61,
    A = 65, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Escape = 256, Enter = 257, Tab = 258, Backspace = 259,
    Right = 262, Left = 263, Down = 264, Up = 265,
    F1 = 290, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    LeftShift = 340, LeftControl = 341, LeftAlt = 342,
    RightShift = 344, RightControl = 345, RightAlt = 346,
    Count = 512
};

enum class Button : int {
    Unknown = 0,
    LeftFaceUp = 1, LeftFaceRight, LeftFaceDown, LeftFaceLeft,          // d-pad
    RightFaceUp = 5, RightFaceRight, RightFaceDown, RightFaceLeft,      // Y B A X
    LeftTrigger1 = 9, LeftTrigger2, RightTrigger1, RightTrigger2,       // LB LT RB RT
    MiddleLeft = 13, Middle, MiddleRight,                               // View, Xbox, Menu
    LeftThumb = 16, RightThumb,
    Count = 18
};

enum class Axis : int { LeftX = 0, LeftY, RightX, RightY, LeftTrigger, RightTrigger, Count };

constexpr int kMaxPads = 4;

bool  KeyDown(Key k);
bool  KeyPressed(Key k);
bool  KeyPressedRepeat(Key k);   // pressed, or auto-repeating while held
int   NextChar();                // typed character queue (Unicode code point); 0 when empty
int   NextKey();                 // pressed key queue as Key values; 0 when empty

bool  PadConnected(int pad);
bool  ButtonDown(int pad, Button b);
bool  ButtonPressed(int pad, Button b);
float AxisValue(int pad, Axis a);   // -1..1 for sticks (up is negative), 0..1 for triggers

// For the platform backend only.
namespace detail {
void NewFrame();                              // current becomes previous; clears the queues' "pressed" edges
void OnKey(Key k, bool down, bool repeat);
void OnChar(int codepoint);
void PollPads();
} // namespace detail

} // namespace rf::input

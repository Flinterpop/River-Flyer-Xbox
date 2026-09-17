// Gamepad polling on the PC through XInput. On a Developer Mode Xbox the
// same job is done by uwp/Pads_Uwp.cpp with Windows.Gaming.Input.
#include "../input/InputState.h"

#include <windows.h>
#include <Xinput.h>

namespace rf::input::detail {

namespace {

constexpr int kRetryFrames = 60;   // XInputGetState on an empty slot is slow: retry once a second

float Stick(SHORT raw)
{
    return static_cast<float>(raw) / 32767.0f;
}

void Read(Pad& p, const XINPUT_GAMEPAD& x)
{
    const WORD b = x.wButtons;
    SetButton(p, Button::LeftFaceUp,     (b & XINPUT_GAMEPAD_DPAD_UP) != 0);
    SetButton(p, Button::LeftFaceRight,  (b & XINPUT_GAMEPAD_DPAD_RIGHT) != 0);
    SetButton(p, Button::LeftFaceDown,   (b & XINPUT_GAMEPAD_DPAD_DOWN) != 0);
    SetButton(p, Button::LeftFaceLeft,   (b & XINPUT_GAMEPAD_DPAD_LEFT) != 0);
    SetButton(p, Button::RightFaceUp,    (b & XINPUT_GAMEPAD_Y) != 0);
    SetButton(p, Button::RightFaceRight, (b & XINPUT_GAMEPAD_B) != 0);
    SetButton(p, Button::RightFaceDown,  (b & XINPUT_GAMEPAD_A) != 0);
    SetButton(p, Button::RightFaceLeft,  (b & XINPUT_GAMEPAD_X) != 0);
    SetButton(p, Button::LeftTrigger1,   (b & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0);
    SetButton(p, Button::RightTrigger1,  (b & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0);
    SetButton(p, Button::LeftTrigger2,   x.bLeftTrigger  > XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
    SetButton(p, Button::RightTrigger2,  x.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
    SetButton(p, Button::MiddleLeft,     (b & XINPUT_GAMEPAD_BACK) != 0);
    SetButton(p, Button::MiddleRight,    (b & XINPUT_GAMEPAD_START) != 0);
    SetButton(p, Button::LeftThumb,      (b & XINPUT_GAMEPAD_LEFT_THUMB) != 0);
    SetButton(p, Button::RightThumb,     (b & XINPUT_GAMEPAD_RIGHT_THUMB) != 0);

    // raylib convention: stick up is negative Y.
    SetAxis(p, Axis::LeftX,        Stick(x.sThumbLX));
    SetAxis(p, Axis::LeftY,        -Stick(x.sThumbLY));
    SetAxis(p, Axis::RightX,       Stick(x.sThumbRX));
    SetAxis(p, Axis::RightY,       -Stick(x.sThumbRY));
    SetAxis(p, Axis::LeftTrigger,  static_cast<float>(x.bLeftTrigger) / 255.0f);
    SetAxis(p, Axis::RightTrigger, static_cast<float>(x.bRightTrigger) / 255.0f);
}

} // namespace

void PollPads()
{
    for (int i = 0; i < kMaxPads; ++i) {
        Pad& p = PadState(i);
        if (!p.connected) {
            if (p.retry > 0) { --p.retry; continue; }
            p.retry = kRetryFrames;
        }
        XINPUT_STATE s {};
        p.connected = (XInputGetState(static_cast<DWORD>(i), &s) == ERROR_SUCCESS);
        if (p.connected) { Read(p, s.Gamepad); } else { ClearPad(p); }
    }
}

} // namespace rf::input::detail

// Gamepad polling on a Developer Mode Xbox through Windows.Gaming.Input.
// Readings are already normalised: sticks -1..1 with up positive (negated
// here to match the raylib convention), triggers 0..1.
#include "../input/InputState.h"

#include <winrt/base.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Gaming.Input.h>

namespace rf::input::detail {

namespace {

using winrt::Windows::Gaming::Input::Gamepad;
using winrt::Windows::Gaming::Input::GamepadButtons;
using winrt::Windows::Gaming::Input::GamepadReading;

constexpr double kTriggerPressed = 0.12;   // XInput's threshold (30/255), as a fraction

bool Has(GamepadButtons all, GamepadButtons b)
{
    return (all & b) == b;
}

void Read(Pad& p, const GamepadReading& r)
{
    const GamepadButtons b = r.Buttons;
    SetButton(p, Button::LeftFaceUp,     Has(b, GamepadButtons::DPadUp));
    SetButton(p, Button::LeftFaceRight,  Has(b, GamepadButtons::DPadRight));
    SetButton(p, Button::LeftFaceDown,   Has(b, GamepadButtons::DPadDown));
    SetButton(p, Button::LeftFaceLeft,   Has(b, GamepadButtons::DPadLeft));
    SetButton(p, Button::RightFaceUp,    Has(b, GamepadButtons::Y));
    SetButton(p, Button::RightFaceRight, Has(b, GamepadButtons::B));
    SetButton(p, Button::RightFaceDown,  Has(b, GamepadButtons::A));
    SetButton(p, Button::RightFaceLeft,  Has(b, GamepadButtons::X));
    SetButton(p, Button::LeftTrigger1,   Has(b, GamepadButtons::LeftShoulder));
    SetButton(p, Button::RightTrigger1,  Has(b, GamepadButtons::RightShoulder));
    SetButton(p, Button::LeftTrigger2,   r.LeftTrigger  > kTriggerPressed);
    SetButton(p, Button::RightTrigger2,  r.RightTrigger > kTriggerPressed);
    SetButton(p, Button::MiddleLeft,     Has(b, GamepadButtons::View));
    SetButton(p, Button::MiddleRight,    Has(b, GamepadButtons::Menu));
    SetButton(p, Button::LeftThumb,      Has(b, GamepadButtons::LeftThumbstick));
    SetButton(p, Button::RightThumb,     Has(b, GamepadButtons::RightThumbstick));

    SetAxis(p, Axis::LeftX,        static_cast<float>(r.LeftThumbstickX));
    SetAxis(p, Axis::LeftY,        -static_cast<float>(r.LeftThumbstickY));
    SetAxis(p, Axis::RightX,       static_cast<float>(r.RightThumbstickX));
    SetAxis(p, Axis::RightY,       -static_cast<float>(r.RightThumbstickY));
    SetAxis(p, Axis::LeftTrigger,  static_cast<float>(r.LeftTrigger));
    SetAxis(p, Axis::RightTrigger, static_cast<float>(r.RightTrigger));
}

} // namespace

void PollPads()
{
    const auto pads = Gamepad::Gamepads();   // snapshot of what is connected right now
    const int  n    = static_cast<int>(pads.Size());
    for (int i = 0; i < kMaxPads; ++i) {
        Pad& p = PadState(i);
        if (i >= n) {
            p.connected = false;
            ClearPad(p);
            continue;
        }
        p.connected = true;
        Read(p, pads.GetAt(static_cast<uint32_t>(i)).GetCurrentReading());
    }
}

} // namespace rf::input::detail

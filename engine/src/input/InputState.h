#pragma once

#include <array>

#include "rf/Input.h"

// Shared between Input_Common.cpp (keyboard state, queues, all the queries)
// and the pad pollers (Pads_XInput.cpp on the PC, Pads_Uwp.cpp on a
// Developer Mode Xbox). A poller fills 'cur' and 'axes' each frame; the
// common code rolls 'cur' into 'prev' and answers the questions.
namespace rf::input::detail {

constexpr int kButtons = static_cast<int>(Button::Count);
constexpr int kAxes    = static_cast<int>(Axis::Count);

struct Pad {
    bool connected {false};
    int  retry {0};                            // frames until a disconnected slot is probed again
    std::array<bool, kButtons> cur {};
    std::array<bool, kButtons> prev {};
    std::array<float, kAxes>   axes {};
};

Pad& PadState(int pad);                        // 0 <= pad < kMaxPads

inline void SetButton(Pad& p, Button b, bool down)
{
    p.cur[static_cast<size_t>(static_cast<int>(b))] = down;
}

inline void SetAxis(Pad& p, Axis a, float v)
{
    p.axes[static_cast<size_t>(static_cast<int>(a))] = (v < -1.0f) ? -1.0f : ((v > 1.0f) ? 1.0f : v);
}

inline void ClearPad(Pad& p)
{
    p.cur.fill(false);
    p.axes.fill(0.0f);
}

} // namespace rf::input::detail

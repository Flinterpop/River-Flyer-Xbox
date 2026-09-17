// Keyboard state, the per-frame queues, the pad tables and every query in
// rf/Input.h. Platform code feeds it through the detail:: hooks; the pad
// pollers fill the Pad tables.
#include "InputState.h"

#include <array>
#include <cassert>

namespace rf::input {

namespace {

constexpr int kKeys  = static_cast<int>(Key::Count);
constexpr int kQueue = 16;   // typed chars / pressed keys remembered per frame

struct Queue {
    std::array<int, kQueue> items {};
    int count {0};
    int head {0};
};

struct State {
    std::array<bool, kKeys> cur {};
    std::array<bool, kKeys> prev {};
    std::array<bool, kKeys> repeat {};
    Queue chars;
    Queue keys;
    std::array<detail::Pad, kMaxPads> pads {};
};

State g;

bool ValidKey(Key k)
{
    const int i = static_cast<int>(k);
    return i > 0 && i < kKeys;
}

bool ValidPad(int pad)
{
    return pad >= 0 && pad < kMaxPads;
}

void Push(Queue& q, int value)
{
    if (q.count >= kQueue) { return; }   // a frame with more than 16 events drops the rest
    q.items[static_cast<size_t>((q.head + q.count) % kQueue)] = value;
    ++q.count;
}

int Pop(Queue& q)
{
    if (q.count == 0) { return 0; }
    const int v = q.items[static_cast<size_t>(q.head)];
    q.head = (q.head + 1) % kQueue;
    --q.count;
    return v;
}

} // namespace

bool KeyDown(Key k)          { return ValidKey(k) && g.cur[static_cast<size_t>(k)]; }
bool KeyPressed(Key k)       { return ValidKey(k) && g.cur[static_cast<size_t>(k)] && !g.prev[static_cast<size_t>(k)]; }
bool KeyPressedRepeat(Key k) { return KeyPressed(k) || (ValidKey(k) && g.repeat[static_cast<size_t>(k)]); }
int  NextChar()              { return Pop(g.chars); }
int  NextKey()               { return Pop(g.keys); }

bool PadConnected(int pad)
{
    return ValidPad(pad) && g.pads[static_cast<size_t>(pad)].connected;
}

bool ButtonDown(int pad, Button b)
{
    const int i = static_cast<int>(b);
    if (!PadConnected(pad) || i <= 0 || i >= detail::kButtons) { return false; }
    return g.pads[static_cast<size_t>(pad)].cur[static_cast<size_t>(i)];
}

bool ButtonPressed(int pad, Button b)
{
    const int i = static_cast<int>(b);
    if (!PadConnected(pad) || i <= 0 || i >= detail::kButtons) { return false; }
    const detail::Pad& p = g.pads[static_cast<size_t>(pad)];
    return p.cur[static_cast<size_t>(i)] && !p.prev[static_cast<size_t>(i)];
}

float AxisValue(int pad, Axis a)
{
    const int i = static_cast<int>(a);
    if (!PadConnected(pad) || i < 0 || i >= detail::kAxes) { return 0.0f; }
    return g.pads[static_cast<size_t>(pad)].axes[static_cast<size_t>(i)];
}

namespace detail {

Pad& PadState(int pad)
{
    assert(ValidPad(pad));
    return g.pads[static_cast<size_t>(pad)];
}

void NewFrame()
{
    g.prev = g.cur;
    g.repeat.fill(false);
    g.chars = Queue {};
    g.keys  = Queue {};
    for (Pad& p : g.pads) { p.prev = p.cur; }
}

void OnKey(Key k, bool down, bool repeat)
{
    if (!ValidKey(k)) { return; }
    const size_t i = static_cast<size_t>(k);
    if (down) {
        if (!g.cur[i]) { Push(g.keys, static_cast<int>(k)); }
        g.cur[i] = true;
        if (repeat) { g.repeat[i] = true; }
    } else {
        g.cur[i] = false;
    }
}

void OnChar(int codepoint)
{
    assert(codepoint >= 32);
    Push(g.chars, codepoint);
}

} // namespace detail

} // namespace rf::input

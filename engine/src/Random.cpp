#include "rf/Random.h"

#include <cassert>

namespace rf::random {

namespace {
unsigned int g_state = 0x9E3779B9u;   // never zero: xorshift would stick there
}

void Seed(unsigned int seed)
{
    g_state = (seed != 0u) ? seed : 0x9E3779B9u;
    assert(g_state != 0u);
}

int Range(int min, int max)
{
    assert(min <= max);
    unsigned int x = g_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    g_state = x;
    const unsigned int span = static_cast<unsigned int>(max - min) + 1u;
    const int r = min + static_cast<int>(x % span);
    assert(r >= min && r <= max);
    return r;
}

} // namespace rf::random

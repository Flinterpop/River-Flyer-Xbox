#pragma once

// Deterministic, seedable integer random numbers (xorshift). The game only
// ever asks for a bounded integer, so that is the whole API.
namespace rf::random {

void Seed(unsigned int seed);
int  Range(int min, int max);   // inclusive both ends; min <= max

} // namespace rf::random

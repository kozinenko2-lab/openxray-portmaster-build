#pragma once
#include <cstdint>

// Exact PRNG used by the Microsoft C runtime linked into the original game.
// AirXonix.wrp.exe keeps the state at 1 and advances it with the classic
// Visual C++ formula. Keeping it explicit is important because random-call
// order affects enemy placement, subtype selection and bonus behaviour.
class LegacyRandom {
public:
    explicit LegacyRandom(std::uint32_t state = 1u) : state_(state) {}
    void seed(std::uint32_t state = 1u) { state_ = state; }
    std::uint32_t state() const { return state_; }
    int next();
    int mask(int mask) { return next() & mask; }
private:
    std::uint32_t state_;
};

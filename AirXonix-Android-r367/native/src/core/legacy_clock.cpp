#include "legacy_clock.hpp"
#include <algorithm>
#include <cmath>

LegacyClock::LegacyClock() : previous_(SDL_GetTicks()) {}

int LegacyClock::smoothedRealMs() const {
    const Uint32 sum = delta_[0] + delta_[1] + delta_[2] + delta_[3];
    return static_cast<int>(std::min<Uint32>(sum >> 2, 100));
}

int LegacyClock::tick(float timeScale) {
    Uint32 now = SDL_GetTicks();
    Uint32 elapsed = now - previous_;
    // The original busy-spins. Yielding is equivalent for game time and much nicer
    // on a battery-powered handheld.
    while (elapsed < 6) {
        SDL_Delay(1);
        now = SDL_GetTicks();
        elapsed = now - previous_;
    }
    previous_ = now;
    delta_[3] = delta_[2];
    delta_[2] = delta_[1];
    delta_[1] = delta_[0];
    delta_[0] = elapsed;
    return static_cast<int>(std::floor(static_cast<float>(smoothedRealMs()) * timeScale));
}

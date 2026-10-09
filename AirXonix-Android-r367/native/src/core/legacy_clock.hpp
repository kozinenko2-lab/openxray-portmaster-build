#pragma once
#include <array>
#include <cstdint>
#include <SDL.h>

// Reproduces AirXonix 0x405FD0 + 0x405F90 + 0x4194A0:
// - no sample shorter than 6 ms
// - four-sample moving average
// - average clamped to 100 ms
// - difficulty/time scale applied after smoothing and truncated to integer ms.
class LegacyClock {
public:
    LegacyClock();
    int tick(float timeScale);
    int smoothedRealMs() const;
private:
    Uint32 previous_ = 0;
    std::array<Uint32,4> delta_{{10,10,10,10}};
};

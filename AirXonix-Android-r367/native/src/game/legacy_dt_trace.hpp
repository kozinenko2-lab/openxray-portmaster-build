#pragma once
#include <cstdint>

struct LegacyScaledDtTrace {
    std::uint32_t wrapper=0x004194A0u;
    std::uint32_t smoother=0x00405F90u;
    std::uint32_t rawSampler=0x00405FD0u;
    std::uint32_t scaleAddress=0x0257D9E8u;
    std::uint32_t outputAddress=0x0257DA7Cu;
    int minimumRawSampleMs=6;
    int movingAverageSamples=4;
    int maximumAverageMs=100;
};
inline constexpr LegacyScaledDtTrace kLegacyScaledDtTrace{};

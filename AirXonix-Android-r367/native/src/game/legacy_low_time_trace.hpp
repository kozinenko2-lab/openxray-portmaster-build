#pragma once
#include <cstdint>

struct LegacyLowTimeTrace {
    std::uint32_t gameplayRoutine=0x004195D0u;
    std::uint32_t timerUpdate=0x004196EEu;
    std::uint32_t warningThresholdCompare=0x004196FDu;
    std::uint32_t warningStart=0x0041971Bu;
    std::uint32_t warningUpdate=0x00419753u;
    std::uint32_t warningStop=0x004197CAu;
    std::uint32_t scoreDecayBlock=0x004197D8u;
    std::uint32_t timerAddress=0x0257DA14u;
    std::uint32_t scoreAddress=0x0257DA20u;
    std::uint32_t warningHandleAddress=0x0044170Cu;
    int warningThreshold=0x2AF8;
    std::uint32_t warningLogicalSfxId=0x12u; // "tick"
    float timerToSourceZ=1.9999999494757503e-5f; // 0x43B4B4
    int scoreDecayPeriodMs=1000;
    int scoreDecayPoints=50;
};
inline constexpr LegacyLowTimeTrace kLegacyLowTimeTrace{};

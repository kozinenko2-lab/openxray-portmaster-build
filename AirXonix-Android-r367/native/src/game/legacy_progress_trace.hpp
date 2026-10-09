#pragma once
#include <cstdint>

// Direct AirXonix.wrp.exe transcription, r101: 0x004194D0.
struct LegacyProgressUpdateTrace {
    std::uint32_t routine = 0x004194D0u;
    std::uint32_t fieldBase = 0x025849DCu;
    int fieldCells = 4096;
    std::uint8_t occupancyMask = 0x3Fu; // excludes active trail 0x40

    std::uint32_t initialOccupiedAddress = 0x0257DA04u;
    std::uint32_t previousOccupiedAddress = 0x0257DA08u;
    std::uint32_t denominatorAddress = 0x0257DA00u;
    std::uint32_t capturePercentAddress = 0x0257DA1Cu;
    std::uint32_t scoreAddress = 0x0257DA20u;
    std::uint32_t bonusAccumulatorAddress = 0x0257DA24u;
    std::uint32_t scorePerCellAddress = 0x0257D9F0u;
    std::uint32_t livesAddress = 0x0257DA10u;

    int percentScale = 100;
    int bonusCellsPerLife = 400;
    int extraLifePickupIndex = 2;
    std::uint32_t extraLifePickupDelayAddress = 0x0254A220u;
    int forcedDelayMs = 1000;
};
inline constexpr LegacyProgressUpdateTrace kLegacyProgressUpdate{};

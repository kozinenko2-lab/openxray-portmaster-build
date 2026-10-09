#pragma once
#include <cstdint>

// Direct AirXonix.wrp.exe transcription, r110: 0x418DD0 per-level initializer.
struct LegacyLevelInitTrace {
    std::uint32_t routine = 0x00418DD0u;
    std::uint32_t levelRecordBytes = 28u;
    std::uint32_t fieldBase = 0x025849DCu;
    int fieldCells = 4096;

    std::uint32_t scoreMultiplierAddress = 0x0257D9F0u;
    float crawlerWeight = 0.699999988079071f;
    float airborneBWeight = 1.5f;
    float specialWeight = 3.0f;
    float baseScoreTerm = 10.0f;
    float finalScoreScale = 0.05000000074505806f;

    std::uint32_t captureDenominatorAddress = 0x0257DA00u;
    std::uint32_t initialOccupiedAddress = 0x0257DA04u;
    std::uint32_t previousOccupiedAddress = 0x0257DA08u;
    std::uint32_t capturePercentAddress = 0x0257DA1Cu;
    int enemyACapturePenalty = 100;
    int enemyBCapturePenalty = 120;
    int eraserCapturePenalty = 100;

    std::uint32_t baseTimeSecondsAddress = 0x0257D9F8u;
    std::uint32_t levelTimerAddress = 0x0257DA14u;
    int timerShift = 10;

    std::uint32_t pickupRespawn = 0x004177C0u;
    std::uint32_t pickupTimerBase = 0x0254A1D4u;
    int pickupRecordStride = 24;
    int pickupCount = 6;
    int initialPickupDelayMs = 5000;

    std::uint32_t homingReset = 0x00415440u;
    std::uint32_t eraserReset = 0x00414DD0u;
    std::uint32_t brightnessAddress = 0x0053B684u;
    float initialBrightness = 1.0f;
    std::uint32_t effectRecoveryAddress = 0x0257DADCu;
    float initialEffectRecovery = 0.029999999329447746f;
};
inline constexpr LegacyLevelInitTrace kLegacyLevelInit{};

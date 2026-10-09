#pragma once
#include <cstdint>

// Direct AirXonix.wrp.exe transcription, r186.
// Core session globals used by 0x418D10 / 0x418DD0 / 0x4194A0 / 0x4195D0.
struct LegacyGameplayStateTrace {
    std::uint32_t sessionBootstrap = 0x00418D10u;
    std::uint32_t levelInitializer = 0x00418DD0u;
    std::uint32_t progressUpdate = 0x004194A0u;
    std::uint32_t gameplayLoop = 0x004195D0u;
    std::uint32_t nonFinalLevelComplete = 0x0041A980u;
    std::uint32_t finalModeComplete = 0x0041B2A0u;

    std::uint32_t livesAddress = 0x0257DA10u;
    std::uint32_t timeRemainingMsAddress = 0x0257DA14u;
    std::uint32_t currentLevelNumberAddress = 0x0257DA18u;
    std::uint32_t capturedPercentAddress = 0x0257DA1Cu;
    std::uint32_t scoreAddress = 0x0257DA20u;
    std::uint32_t trailActiveAddress = 0x0257DA74u;

    int initialLives = 3;
    int initialLevelNumber = 0; // incremented by 0x418DD0, so first loaded level becomes 1
    int baseTimeSeconds = 60;
    int timerStorageScale = 1024; // initializer stores baseTimeSeconds << 10
    int levelCompletePercent = 100;

    // 0x4195D0 return contract.
    int nextLevelReturn = 1;
    int abortedReturn = -1;

    // Final-mode bonus terms proven at 0x41B2B7+.
    int finalTimePointsPerSecond = 50; // timeRemainingMs / 20
    int finalLifeBonusBase = 5000;     // multiplied by lives and gameTimeScale
    int completionBonusBase = 5000;    // final mode only, multiplied by gameTimeScale
    int overCaptureBonusPerPercent = 1000; // (capturedPercent-100) * gameTimeScale
    int interLevelDurationMs = 4000;
    int finaleInputUnlockMs = 7000;
    int finaleAutoExitMs = 90000;

    static int timeBonus(int timeRemainingMs){ return timeRemainingMs/20; }
};
inline constexpr LegacyGameplayStateTrace kLegacyGameplayStateTrace{};

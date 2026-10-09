#pragma once
#include <array>
#include <cstdint>

// Direct AirXonix.wrp.exe transcription, r109.
// 0x418D10 is the campaign/session bootstrap that runs once before the first
// per-level initializer (0x418DD0). It must not be conflated with level reload.
struct LegacySessionBootstrapTrace {
    std::uint32_t routine = 0x00418D10u;
    std::uint32_t levelInitializer = 0x00418DD0u;
    std::uint32_t caller = 0x00424E57u;

    std::uint32_t difficultyScaleAddress = 0x0257D9E8u;
    std::uint32_t modeLevelCountAddress = 0x0257D9ECu;
    std::uint32_t baseTimeSecondsAddress = 0x0257D9F8u;
    std::uint32_t currentLevelAddress = 0x0257DA18u;
    std::uint32_t scoreAddress = 0x0257DA20u;
    std::uint32_t bonusAccumulatorAddress = 0x0257DA24u;
    std::uint32_t livesAddress = 0x0257DA10u;

    int initialLives = 3;
    int initialScore = 0;
    int initialLevel = 0;
    int baseTimeSeconds = 60;
    std::array<unsigned,5> modeLevelCounts{{7,15,20,20,20}};

    std::uint32_t cameraXAddress = 0x0257DA58u;
    std::uint32_t cameraYAddress = 0x0257DA5Cu;
    std::uint32_t cameraZAddress = 0x0257DA60u;
    std::uint32_t cameraAngle1Address = 0x0257DA68u;
    std::uint32_t cameraAngle2Address = 0x0257DA6Cu;
    std::uint32_t cameraAngle3Address = 0x0257DA70u;
    float initialCameraX = 0.5f;
    float initialCameraY = 0.103f;
    float initialCameraZ = 0.35f;
    int initialCameraAngle1 = 0;
    int initialCameraAngle2 = -302;
    int initialCameraAngle3 = 0;

    std::uint32_t audioOrientationSetup = 0x0040A3D0u;
    std::uint32_t audioBasisBuild = 0x0040A450u;
    std::uint32_t audioListenerPosition = 0x0040A5C0u;
    float audioConePi = 3.1415927410125732f;
    float audioConeScale = 1.5f;
    float audioConeBias = 0.10000000149011612f;
    std::array<int,3> audioBasisAngles{{0,-302,0}};

    std::uint32_t pickupVisualReset = 0x004156A0u;
    std::uint32_t pickupVisualBase = 0x0257DB04u;
    int pickupVisualStride = 0x10;
    int pickupVisualCount = 6;
    float pickupVisualInitial = 0.5f;

    // r185 correction: these were previously mislabeled as clock state.
    // Direct xrefs prove they are the registration/integrity sentinel copied
    // into three shadows during session bootstrap. Native gameplay must not
    // treat them as timing state.
    std::uint32_t integritySentinelAddress = 0x025459B4u;
    std::array<std::uint32_t,3> integrityShadowAddresses{{0x02545960u,0x02545964u,0x02545968u}};
};
inline constexpr LegacySessionBootstrapTrace kLegacySessionBootstrap{};

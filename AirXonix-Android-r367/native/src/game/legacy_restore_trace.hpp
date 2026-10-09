#pragma once
#include <cstdint>

struct LegacySessionRestoreTrace {
    std::uint32_t routine=0x00419250u;
    std::uint32_t onlyCaller=0x00424E7Eu;
    std::uint32_t levelAddress=0x0257DA18u;
    std::uint32_t scoreAddress=0x0257DA20u;
    std::uint32_t livesAddress=0x0257DA10u;
    std::uint32_t restoreFlag=0x025B789Cu;
    std::uint32_t savedLevel=0x025B5B44u;
    std::uint32_t savedScore=0x025B78A0u;
    std::uint32_t savedLives=0x025B5B50u;
    std::uint32_t checkpointWriter=0x004192A3u;
};
inline constexpr LegacySessionRestoreTrace LegacySessionRestore{};

struct LegacyAbortConfirmTrace {
    std::uint32_t routine=0x0041E520u;
    std::uint32_t caller=0x0041983Cu;
    std::uint32_t panelPrepared=0x0257F5DCu; // cinematic slot 4 = ABOR
    float enterStartY=-0.25f;
    float holdY=-0.03f;
    float enterRatePerMs=0.0007999999797903001f;
    // r304 DIRECT EXE 0x41E630..0x41E6E2: ABOR owns a green-family
    // scene light. Before/for No: (g,255,g), g=165-y*300. For Yes while
    // leaving: (a,b,a), a=y*566.6666259765625+156, b=y*849.9999389648438+255.
    float lightBase=165.0f;
    float lightPanelScale=300.0f;
    float yesOuterScale=566.6666259765625f;
    float yesOuterBase=156.0f;
    float yesGreenScale=849.9999389648438f;
    float yesGreenBase=255.0f;
    constexpr float normalGray(float y) const { return lightBase-y*lightPanelScale; }
    constexpr float yesOuter(float y) const { return y*yesOuterScale+yesOuterBase; }
    constexpr float yesGreen(float y) const { return y*yesGreenScale+yesGreenBase; }
    std::uint8_t yesKey='Y';
    std::uint8_t enterKey=0x0D;
    std::uint8_t noKey='N';
    std::uint8_t escapeKey=0x1B;
    std::uint8_t clickSfx=0x16;
};
inline constexpr LegacyAbortConfirmTrace LegacyAbortConfirm{};

// r185 DIRECT EXE: the checkpoint fields above belong to the Backspace restart
// path, not to a persistent save-game system. 0x419270 writes the checkpoint,
// sets both flags, and marks 0x257DAD0 so 0x41E520 returns success immediately
// without presenting ABOR. 0x424F2F sees gameplay result -1 plus restartFlag
// and jumps back to 0x424E2D, where 0x419250 restores the checkpoint.
struct LegacyRestartCurrentLevelTrace {
    static constexpr std::uint32_t inputRoutine=0x00419270u;
    static constexpr std::uint8_t keyboardKey=0x08u; // Backspace
    static constexpr std::uint32_t restartFlag=0x025B5B9Cu;
    static constexpr std::uint32_t restorePending=0x025B789Cu;
    static constexpr std::uint32_t restartCredits=0x025B5B60u;
    static constexpr std::uint32_t checkpointWriter=0x004192A3u;
    static constexpr std::uint32_t immediateAbortMarker=0x0257DAD0u;
    static constexpr std::uint32_t abortModal=0x0041E520u;
    static constexpr std::uint32_t outerRestartBranch=0x00424F2Fu;
    static constexpr std::uint32_t outerRestartTarget=0x00424E2Du;
    static constexpr int initialCredits=5;
};
inline constexpr LegacyRestartCurrentLevelTrace kLegacyRestartCurrentLevelTrace{};

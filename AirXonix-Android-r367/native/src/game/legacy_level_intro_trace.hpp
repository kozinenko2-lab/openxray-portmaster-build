#pragma once
#include <cstdint>
namespace LegacyLevelIntro {
struct Trace {
    static constexpr std::uintptr_t dispatcher=0x0041CEA0u;
    static constexpr std::uintptr_t capableRoutine=0x0041D4D0u;
    static constexpr std::uintptr_t fallbackRoutine=0x0041DBC0u;
    static constexpr std::uintptr_t frameDtRoutine=0x004194A0u;
    static constexpr std::uintptr_t worldFrameBegin=0x0041D89Fu;
    static constexpr std::uintptr_t overlayBegin=0x0041DAB0u;
    static constexpr std::uintptr_t lev2Submit=0x0041DB0Au;
    static constexpr std::uintptr_t cnt3Submit=0x0041DB58u;
    static constexpr int durationMs=3000;
    static constexpr int overlayMotionThresholdMs=1000;
    static constexpr int initialCameraAngle2=-450;
    static constexpr float initialWorldX=0.50156247615814209f; // 0x3F006667
    static constexpr float initialWorldY=0.31999999284744263f; // 0x3EA3D70A
    static constexpr float initialWorldZ=0.39218750596046448f; // 0x3EC8CCCD
    static constexpr float initialCameraX=0.5f;
    static constexpr float initialCameraY=0.42300000786781311f; // 0x3ED89374
    static constexpr float initialCameraZ=0.40000000596046448f;
    // DIRECT EXE capable 0x41D5CC/0x41D5E0 and fallback
    // 0x41DC1C/0x41DBF9 keep LEV2 angle and plaque Y as separate locals.
    static constexpr float initialPlaqueY=-0.10000000149011612f;
    static constexpr float initialPlaqueAngleRad=0.f;
    static constexpr float earlyPlaqueYStepPerMs=0.00009999999747378752f;
    static constexpr float latePlaqueYStepPerMs=-0.00005999999848427251f;
    static constexpr float latePlaqueAngleStepRadPerMs=0.009999999776482582f;
    static constexpr float lateDigitXStepPerMs=1.9999999494757503e-5f;
    static constexpr float lightMin=0.f;                  // 0x43B264
    static constexpr float lightMax=255.f;                // 0x43B3DC / 0x437F0000
    static constexpr float lightVelocityPerMs=0.13500000536441803f; // 0x3E0A3D71
    static constexpr std::uintptr_t lightSeed=0x0041D50Eu;
    static constexpr std::uintptr_t lightIntegrate=0x0041D627u;
    static constexpr std::uintptr_t lightReverse=0x0041D648u;
    static constexpr std::uintptr_t lightExit=0x0041D673u;
    static constexpr float worldYStepPerMs=0.00005999999848427251f; // 0x43B570
    static constexpr float lateMotionPerMs=0.00009999999747378752f; // 0x43B350
    static constexpr float lateMotionClamp=-0.02500000037252903f; // 0x43B56C
    static constexpr bool rendersWorldBeforeOverlay=true;
    static constexpr bool usesLev2=true;
    static constexpr bool usesCnt3Digits=true;
};
inline constexpr Trace kTrace{};
}

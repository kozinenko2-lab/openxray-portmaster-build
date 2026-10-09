#pragma once
#include <cstdint>

// r293 DIRECT EXE: 0x45000C is not a gameplay/presentation preference. It is
// the D3D capability result labelled "Blending supported" by startup code.
// 0x40840D clears it, 0x408413 queries D3DCAPS, 0x408423/0x40842C require
// bit 2 in two capability bytes, then 0x408440 sets it to 1. 0x41CEA0 uses
// this flag to choose the additive/blended LevelIntro backend 0x41D4D0;
// zero chooses legacy fallback 0x41DBC0. OpenGL ES 2.0 guarantees the blend
// machinery used by the native renderer, so the PortMaster runtime maps to
// the capable path deliberately rather than emulating the obsolete fallback.
struct LegacyLevelIntroBackendTrace {
    std::uint32_t capabilityGlobal=0x0045000Cu;
    std::uint32_t capabilityProbe=0x0040840Du;
    std::uint32_t capabilitySet=0x00408440u;
    std::uint32_t logString=0x0043F610u;
    std::uint32_t dispatcher=0x0041CEA0u;
    std::uint32_t capablePath=0x0041D4D0u;
    std::uint32_t fallbackPath=0x0041DBC0u;
    std::uint8_t requiredBit=0x02u;
    constexpr bool gles2UsesCapablePath() const { return true; }
};
inline constexpr LegacyLevelIntroBackendTrace kLegacyLevelIntroBackendTrace{};

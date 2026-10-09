#pragma once
#include "legacy_mesh.hpp"
#include <cstdint>

namespace LegacySettingsWidgets {

// r154 direct-EXE constructors used by the Settings screen.
// Track: 0x422B6B..0x422C50 -> 0x402A50 -> model 0x257F584.
// Knob:  0x421647..0x421713 -> 0x402A50 -> model 0x257F580,
//        prepared handle 0x2583754.
struct Trace {
    std::uint32_t trackConstructor=0x00422B6Bu;
    std::uint32_t trackBuilder=0x00402A50u;
    std::uint32_t trackModel=0x0257F584u;
    std::uint32_t trackPrepared=0x0257F5C8u;
    std::uint32_t knobConstructor=0x00421647u;
    std::uint32_t knobBuilder=0x00402A50u;
    std::uint32_t knobModel=0x0257F580u;
    std::uint32_t knobPrepared=0x02583754u;
    std::uint32_t trackSubmit=0x00413F38u;
    std::uint32_t knobSubmit=0x004140C1u;
    int radialSegments=16;
    float trackSubmitX=0.008999999612569809f;
    float trackScale=0.1599999964237213f;
    float knobScale=0.15000000596046448f;
};
inline constexpr Trace kTrace{};

LegacyMesh buildTrack();
LegacyMesh buildKnob();

} // namespace LegacySettingsWidgets

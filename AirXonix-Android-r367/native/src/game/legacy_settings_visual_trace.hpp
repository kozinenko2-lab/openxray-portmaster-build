#pragma once
#include <array>
#include <cstdint>

// DIRECT EXE r138: visual submit contract inside 0x00413D02..0x00414132.
struct LegacySettingsVisualTrace {
    std::uint32_t loopBegin=0x00413D02u;
    std::uint32_t labelSubmit=0x00413EE4u;
    std::uint32_t trackSubmit=0x00413F38u;
    std::uint32_t speechSubmit=0x00413FE6u;
    std::uint32_t knobSubmit=0x004140C1u;
    float labelX=-0.006000000052154064f;
    float firstRowZ=-0.004000000189989805f;
    float rowStepZ=-0.00279999990016222f;
    float trackX=0.008999999612569809f;
    float knobCenterX=0.008999999612569809f;
    float knobUnitsToWorld=9.999999747378752e-06f;
    float speechX=0.004999999888241291f;
    float speechZ=-0.012399999424815178f;
    float selectedBrightness=1.f,normalBrightness=.6f;
    float selectedScale=1.15f,normalScale=1.f;
    float brightnessRatePerMs=.002f,scaleRatePerMs=.001f;
    constexpr float rowZ(int row) const { return firstRowZ + float(row)*rowStepZ; }
    constexpr float knobX(float value) const { return knobCenterX + (value-500.f)*knobUnitsToWorld; }
};
inline constexpr LegacySettingsVisualTrace kLegacySettingsVisualTrace{};

#pragma once
#include <cstdint>
struct LegacyPauseTrace {
    std::uint32_t routine=0x0041DE30u;
    std::uint32_t pickupHandleBegin=0x00441718u;
    std::uint32_t pickupHandleEnd=0x00441730u;
    std::uint32_t lowTimeHandle=0x0044170Cu;
    std::uint32_t panelModel=0x0257F5A8u;
    std::uint32_t panelPrepared=0x0257F5D4u;
    float enterStartY=-0.30000001192092896f;
    float holdY=-0.029999999329447746f;
    float exitY=-0.25f;
    float enterRatePerMs=0.0005000000237487257f;
    float leaveRatePerMs=0.00039999998989515007f;
    float rotationRatePerMs=0.004999999888241291f;
    float submitZ=0.008999999612569809f;
    float lightBase=165.0f;
    float lightPanelScale=300.0f;
    constexpr float lightGray(float panelY) const { return lightBase-panelY*lightPanelScale; }
    std::size_t clickSfx=0x16u;
};
inline constexpr LegacyPauseTrace kLegacyPauseTrace{};

#pragma once
#include <cstdint>
namespace LegacyFinaleCinematic {
struct Trace {
    static constexpr std::uint32_t phaseSeed=0x0041B468u;
    static constexpr int phaseSeedMask=7;
    static constexpr std::uint32_t pairBegin=0x0041B94Cu;
    static constexpr std::uint32_t slot5Model=0x0257F5B4u;
    static constexpr std::uint32_t slot5Prepared=0x0257F5E0u;
    static constexpr std::uint32_t slot5Submit=0x0041BA28u;
    static constexpr std::uint32_t slot0Model=0x0257F5A0u;
    static constexpr std::uint32_t slot0Prepared=0x0257F5CCu;
    static constexpr std::uint32_t slot0Submit=0x0041BA3Bu;
    static constexpr int textureSlot=4;
    static constexpr float phasePerMs=0.0020000000949949026f;
    static constexpr float rotXAmplitude=0.35999998450279236f;
    static constexpr float rotZPhaseScale=1.2999999523162842f;
    static constexpr float rotZAmplitude=0.27000001072883606f;
    static constexpr float slot5Z=0.0f;
    static constexpr float slot0Z=-0.00800000037997961f;
    // The original passes the elapsed-ms integer bits as the Y float; for the
    // entire 90 s sequence this is a tiny denormal effectively equal to zero.
    static constexpr float nativeEquivalentY=0.0f;
};
}

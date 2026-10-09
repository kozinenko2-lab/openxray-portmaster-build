#pragma once
#include <array>
#include <cstdint>
#include <cstddef>

namespace LegacyInterLevel {
struct Trace {
    static constexpr std::uintptr_t routine=0x0041A980u;
    static constexpr std::uintptr_t entryGameSfx=0x0041A9F8u;
    static constexpr std::uintptr_t musicFade=0x0041AA1Cu;
    static constexpr std::uintptr_t retainedVintStart=0x0041AA41u;
    static constexpr std::uintptr_t speechThreshold=0x0041AA8Du;
    static constexpr std::uintptr_t fadeThreshold=0x0041AB69u;
    static constexpr std::uintptr_t retainedVintUpdate=0x0041AC6Bu;
    static constexpr std::uintptr_t cameraSetup=0x0041AD54u;
    static constexpr std::uintptr_t listenerSetup=0x0041AD87u;
    static constexpr std::uintptr_t retainedVintStop=0x0041B1EEu;
    static constexpr std::uintptr_t crawlerReset=0x0041B1DDu;
    static constexpr int durationMs=4000;
    static constexpr int speechRemainingThresholdMs=3000;
    static constexpr int fadeRemainingThresholdMs=2550;
    static constexpr int debrisRemainingThresholdMs=2000;
    static constexpr std::size_t entryLogicalSfx=0x19u; // game
    static constexpr std::size_t retainedLogicalSfx=0x07u; // vint
    static constexpr float musicFadePerMs=0.0010000000474974513f;
    static constexpr float speechScalar=1.5f;
    static constexpr std::array<std::size_t,4> speechBySelector{{0x33u,0x2Du,0x2Eu,0x2Fu}}; // cmex, yes1, cool, that
    static constexpr float xonixRisePerMs=0.000022000000171829015f;
    static constexpr float brightnessDivisor=2550.f;
    static constexpr float lightGrayPerRemainingMs=0.10000000149011612f;
    static constexpr std::uintptr_t cinematicPhaseSeed=0x0041AA55u;
    static constexpr std::uintptr_t cinematicTextureSelect=0x0041ADCFu;
    static constexpr int cinematicTextureSlot=4;
    static constexpr std::uintptr_t cinematicPairBegin=0x0041AE13u;
    static constexpr std::uintptr_t cinematicSlot5Submit=0x0041AE59u;
    static constexpr std::uintptr_t cinematicSlot0Submit=0x0041AEACu;
    static constexpr std::uintptr_t cinematicSlot5Model=0x0257F5B4u;
    static constexpr std::uintptr_t cinematicSlot5Prepared=0x0257F5E0u;
    static constexpr std::uintptr_t cinematicSlot0Model=0x0257F5A0u;
    static constexpr std::uintptr_t cinematicSlot0Prepared=0x0257F5CCu;
    static constexpr float cinematicYOffsetInitial=-0.10000000149011612f;
    static constexpr float cinematicYOffsetPerMs=0.00007999999797903001f;
    static constexpr float cinematicYOffsetMax=-0.03799999877810478f;
    static constexpr int cinematicPhaseSeedMask=7;
    static constexpr float cinematicPhasePerMs=0.0020000000949949026f;
    static constexpr float cinematicRotXAmplitude=0.27000001072883606f;
    static constexpr float cinematicRotZPhaseScale=1.2999999523162842f;
    static constexpr float cinematicRotZAmplitude=0.21000000834465027f;
    static constexpr float cinematicSlot5Z=0.004000000189989805f;
    static constexpr float cinematicSlot0Z=-0.004000000189989805f;
    static constexpr float rotorPhasePerMs=0.013000000268220901f;
    static constexpr float rotorRadius=0.003700000001117587f;
};
}

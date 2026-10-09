#pragma once
#include <cstdint>
namespace LegacyFinaleEntry {
struct Trace {
    static constexpr std::uint32_t loopBegin=0x0041B47Cu;
    static constexpr std::uint32_t inputArm=0x0041B4EFu;
    static constexpr std::uint32_t firstFreshEvent=0x0041B52Fu;
    static constexpr std::uint32_t autoExit=0x0041B548u;
    static constexpr std::uint32_t pickupLoop=0x0041B560u;
    static constexpr std::uint32_t lightUpdate=0x0041B5CAu;
    static constexpr std::uint32_t loopExitTest=0x0041B5F7u;
    static constexpr int inputArmMs=7000;
    static constexpr int autoExitMs=90000;
    static constexpr float presentationAngleInitial=-0.11999999731779099f;
    static constexpr float presentationAngleCeiling=-0.029999999329447746f;
    static constexpr float presentationAngleRateIn=1.9999999494757503e-5f;
    static constexpr float presentationAngleRateOut=-2.9999999242136255e-5f;
    static constexpr float presentationYRate=1.9999999494757503e-5f;
    static constexpr float presentationYMax=0.10000000149011612f;
    static constexpr float gameplayYCap=0.04f;
    static constexpr float sceneLightInitial=255.0f;
    static constexpr float sceneLightHold=200.0f;
    static constexpr float sceneLightIntroFadePerMs=0.05000000074505806f;
    static constexpr float sceneLightExitFadePerMs=0.10000000149011612f;
    static constexpr float rotorPhasePerLoopPerSavedSfxMaster=0.013000000268220901f;
};
}

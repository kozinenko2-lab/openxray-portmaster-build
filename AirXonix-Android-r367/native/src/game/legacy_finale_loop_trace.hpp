#pragma once
#include <cstdint>
namespace LegacyFinaleLoopTrace {
inline constexpr std::uint32_t Begin=0x0041B60Cu;
inline constexpr std::uint32_t SfxFade=0x0041B60Cu;
inline constexpr std::uint32_t Orbit=0x0041B645u;
inline constexpr std::uint32_t CameraAngleFollower=0x0041B672u;
inline constexpr std::uint32_t Camera=0x0041B732u;
inline constexpr std::uint32_t LiveWorld=0x0041B7D8u;
inline constexpr std::uint32_t RotatingFieldMesh=0x0041B7F0u;
inline constexpr std::uint32_t RotorPhase=0x0041B838u;
inline constexpr std::uint32_t FrameBegin=0x0041B8F4u;
inline constexpr std::uint32_t CinematicPair=0x0041B94Cu;
inline constexpr std::uint32_t PickupBasePass=0x0041BACDu;
inline constexpr std::uint32_t CrawlerBasePass=0x0041BB0Cu;
inline constexpr std::uint32_t AirborneBasePass=0x0041BB4Bu;
inline constexpr std::uint32_t ReflectionPass=0x0041BBEEu;
inline constexpr std::uint32_t TailEffects=0x0041BCCFu;
inline constexpr std::uint32_t FramePresent=0x0041BD53u;
inline constexpr std::uint32_t Cleanup=0x0041BD5Fu;
inline constexpr float SfxFadeInv255=0.003921568859368563f;
inline constexpr float OrbitPerMs=0.00039999998989515007f;
inline constexpr float CameraFollowerPerMs=0.009999999776482582f;
inline constexpr float CameraFollowerTargetScale=150.0f;
inline constexpr float RotorRadius=0.003700000001117587f;
inline constexpr float RotorPhasePerLoopPerSavedSfxMaster=0.013000000268220901f;
inline constexpr float PairPhasePerMs=0.0020000000949949026f;
inline constexpr float PairRotXAmplitude=0.35999998450279236f;
inline constexpr float PairRotZPhaseScale=1.2999999523162842f;
inline constexpr float PairRotZAmplitude=0.27000001072883606f;
inline constexpr float PickupSubmitYOffset=0.007000000216066837f;
inline constexpr float CrawlerSubmitYOffset=0.013000000268220901f;
inline constexpr float AirborneGlintSize=0.005499999970197678f;
inline constexpr float CrawlerGlintSize=0.0035000001080334187f;
inline constexpr int InputArmMs=7000;
inline constexpr int AutoExitMs=90000;
}

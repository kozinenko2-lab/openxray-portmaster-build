#pragma once
#include <algorithm>
#include <cstdint>

// r231 direct-EXE reconstruction of the Information fullscreen 0x40E3D0 path.
// Pages 0/1 use one D3DTLVERTEX quad, not the two prepared menu layers.
namespace LegacyInformationBackdrop {
inline constexpr std::uint32_t QuadRoutine=0x0040E3D0u;
inline constexpr std::uint32_t Page0QuadCall=0x00410454u;
inline constexpr std::uint32_t Page1QuadCall=0x004106BCu;
inline constexpr int Page0TextureSlot=0;
inline constexpr int Page1TextureSlot=1;
inline constexpr float Page0PhaseStep=0.00019999999494757503f; // 0x43B2F0
inline constexpr float Page1PhaseStep=0.0003000000142492354f;  // 0x43B2E4
inline constexpr float UvExtent=3.0f;
inline constexpr float TlDepth=0.9900000095367432f;           // 0x3F7D70A4
inline constexpr float TlRhw=1.0101009607315063f;             // 0x3F814AFD
inline constexpr int TransitionClamp=0x7C0;
inline constexpr int TransitionRate=3;

inline float wrap01(float v){
    while(v>=1.f)v-=1.f;
    while(v<0.f)v+=1.f;
    return v;
}
inline float advancePhase(int page,float phase,int dtMs){
    const float step=page==0?Page0PhaseStep:Page1PhaseStep;
    return wrap01(phase+float(dtMs)*step);
}
inline int advanceTransition(int value,int dtMs){
    return std::min(TransitionClamp,value+std::max(0,dtMs)*TransitionRate);
}
inline std::uint32_t packedRgb(int transition){
    const std::uint32_t i=static_cast<std::uint32_t>(std::clamp(transition,0,TransitionClamp)>>4)&0xffu;
    return i*0x00010101u;
}
inline float grayscale01(int transition){
    return float((packedRgb(transition))&0xffu)/255.f;
}
}

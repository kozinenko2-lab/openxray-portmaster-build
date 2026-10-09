#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cmath>

// r232 direct-EXE state machine shared by 0x4101B0, 0x4104A0 and 0x414170.
namespace LegacyInformationTransition {
inline constexpr int FadeMax=0x7C0;
inline constexpr int FadeInRate=3;
inline constexpr int FadeOutRate=-4;
inline constexpr std::size_t AdvanceSfx=0x16u;

inline int modelLightByte(int counter){
    return std::clamp(counter,0,FadeMax)>>3;
}
inline float modelLightScale(int counter){
    return float(modelLightByte(counter))/255.f;
}
// 0x41431E..0x4143A3: page-3 lead model alone uses counter>>4; the
// normal counter>>3 diffuse is restored immediately afterward.
inline int leadModelLightByte(int counter){
    return std::clamp(counter,0,FadeMax)>>4;
}
inline float leadModelLightScale(int counter){
    return float(leadModelLightByte(counter))/255.f;
}

// r330 DIRECT EXE 0x410386..0x4103B9, 0x4105EB..0x410625 and
// 0x4142B6..0x4142EE: prompt phase advances by 2*dt and recolours row 14
// with red-only trunc((cos+1)*31+128)<<16 before the text palette fade.
inline int advancePromptPhase(int phase,int dtMs){ return (phase+2*std::max(0,dtMs))&0x7ff; }
inline int promptRedByte(int phase){
    constexpr double twoPi=6.283185307179586476925286766559;
    const double c=std::cos(double(phase&0x7ff)*twoPi/2048.0);
    return std::clamp(static_cast<int>((c+1.0)*31.0+128.0),0,255);
}
inline std::uint32_t promptRgb(int phase){ return std::uint32_t(promptRedByte(phase))<<16; }

inline int textPaletteLevel(int counter){ return std::clamp(counter>>6,0,31); }
inline int fadeTextByte(int byte,int counter){ return (std::clamp(byte,0,255)*textPaletteLevel(counter))/31; }


// r338 DIRECT EXE 0x414403..0x414413: Information page-3 sway phase is
// another unbounded float accumulator. Only the separate special phase at
// 0x4144A0..0x4144C5 is wrapped by 2*pi.
inline float advanceSwayPhase(float phase,int dtMs){
    return phase + float(std::max(0,dtMs))*0.0020000000949949026f;
}

// r337 DIRECT EXE 0x410B12..0x410B36: the Information-page Xonix
// presentation phase is an unbounded float accumulator; the original does not
// wrap it before the trigonometric uses.
inline float advanceXonixPhase(float phase,int dtMs){
    return phase + float(std::max(0,dtMs))*0.004999999888241291f;
}

inline int advanceCounter(int counter,int rate,int dtMs){
    return counter + rate*std::max(0,dtMs);
}
}

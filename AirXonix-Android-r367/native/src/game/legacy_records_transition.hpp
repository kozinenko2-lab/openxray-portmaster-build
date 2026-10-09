#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cmath>

// r234 DIRECT EXE 0x40F3A8..0x40F940: the normal Records browser owns the
// same integer fade law used by the surrounding M1 presentations. It starts
// at zero with +3*dt, clamps at 0x7C0, accepts fresh input only after the
// clamp, and exits only after -4*dt takes the counter below zero.
namespace LegacyRecordsTransition {
inline constexpr int FadeMax=0x7C0;
inline constexpr int FadeInRate=3;
inline constexpr int FadeOutRate=-4;
inline constexpr std::size_t NavigateSfx=0x15u;
inline constexpr std::size_t ExitSfx=0x16u;
inline constexpr std::size_t ModeCount=5u;

inline int advanceCounter(int value,int rate,int dtMs){
    return value+rate*std::max(0,dtMs);
}
inline std::size_t previousMode(std::size_t mode){
    return mode==0u?ModeCount-1u:(mode-1u)%ModeCount;
}
inline std::size_t nextMode(std::size_t mode){
    return (mode+1u)%ModeCount;
}
inline int backdropByte(int counter){
    return std::clamp(counter,0,FadeMax)>>4;
}
inline float backdropGray(int counter){
    return float(backdropByte(counter))/255.f;
}
inline int modelLightByte(int counter){
    return std::clamp(counter,0,FadeMax)>>3;
}
inline float modelLightScale(int counter){
    return float(modelLightByte(counter))/255.f;
}

// r244 DIRECT EXE 0x40F3FA..0x40F42F: while a new high-score name is being
// edited, the 11-bit trig-table phase advances by exactly 5*dt. The first
// float in each 8-byte trig entry is cosine (0x4012F0 uses pair {cos,sin}).
inline int advanceNamePulsePhase(int phase,int dtMs){
    return (phase+5*std::max(0,dtMs))&0x7ff;
}
// r328 DIRECT EXE 0x40FAFD..0x40FB05: fnt4 is rendered through the same
// 32-level palette law used by 0x40BF40, with level=fadeCounter>>6.
inline int textPaletteLevel(int fadeCounter){ return std::clamp(fadeCounter>>6,0,31); }
inline int fadeTextByte(int byte,int fadeCounter){ return (std::clamp(byte,0,255)*textPaletteLevel(fadeCounter))/31; }

// r327 DIRECT EXE 0x40F967..0x40F9BE: the eleven inner heading cells
// (columns 15..25,row 1) use a travelling cosine colour wave.
inline int advanceHeadingPhase(int phase,int dtMs){ return (phase+4*std::max(0,dtMs))&0x7ff; }
inline std::uint32_t headingWaveRgb(int phase,int cell){
    constexpr double twoPi=6.283185307179586476925286766559;
    const int a=(phase+cell*0x190)&0x7ff;
    const double c=std::cos(double(a)*twoPi/2048.0);
    const int v=std::clamp(static_cast<int>((c+1.0)*63.0+128.0),0,255);
    return 0x00ff0000u-std::uint32_t(v)*0x00010100u;
}

inline int namePulseByte(int phase){
    constexpr double twoPi=6.283185307179586476925286766559;
    const double c=std::cos(double(phase&0x7ff)*twoPi/2048.0);
    // 0x40F42A calls 0x43129C: x87 truncate toward zero, not nearest.
    return std::clamp(static_cast<int>((c+1.0)*63.0+128.0),0,255);
}
}

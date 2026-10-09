#pragma once

// r199 DIRECT EXE 0x0041FD50.
// The prepared world-background object at 0x02583600 owns a shared V phase.
// Every live-world frame advances it by dt*0.0001, subtracts 1.0 once when
// phase >= 1, writes phase to the two near vertices and phase+9.6 to the two
// far vertices. 0x4201E0 then derives the camera-dependent inner-ring UVs by
// adding the same phase.
namespace LegacyBackgroundScroll {
inline constexpr float RatePerMs = 0.00009999999747378752f; // 0x43B350
inline constexpr float Wrap = 1.0f;                          // 0x43B280
inline constexpr float FarV = 9.600000381469727f;            // 0x43B598

inline float advance(float phase,int dtMs){
    if(dtMs<=0)return phase;
    phase += static_cast<float>(dtMs)*RatePerMs;
    // Literal x86 behaviour is one subtraction, not fmod(). The engine's
    // normal dt clamp makes more than one wrap per call impossible.
    if(phase>=Wrap)phase-=Wrap;
    return phase;
}
}

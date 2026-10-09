#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace airxonix {
struct LegacySpatialAudioTrace {
    static constexpr std::uintptr_t bootstrapRoutine=0x0040A3D0u;
    static constexpr std::uintptr_t basisRoutine=0x0040A450u;
    static constexpr std::uintptr_t listenerRoutine=0x0040A5C0u;
    static constexpr std::uintptr_t routine=0x0040A7B0u;
    static constexpr std::uintptr_t listenerX=0x00451534u;
    static constexpr std::uintptr_t listenerY=0x00451538u;
    static constexpr std::uintptr_t listenerZ=0x0045153Cu;
    static constexpr std::uintptr_t basisA=0x00451510u;
    static constexpr std::uintptr_t basisB=0x0045151Cu;
    static constexpr std::uintptr_t distanceScale=0x00451530u;
    static constexpr std::uintptr_t sourceScale=0x00451528u;
    static constexpr std::uintptr_t directionalBias=0x0045152Cu;
    static constexpr int channelClamp=0x55;
    static constexpr float bootstrapSourceScale=0.1f;
    static constexpr float bootstrapBias=1.5f;
    static constexpr float bootstrapDistanceScale=25.6f; // 64/(1.5+1)
    static constexpr float combinedScale=2.56f;
};

struct LegacySpatialBasis {
    std::array<float,3> a{{1.f,0.f,0.f}};
    std::array<float,3> b{{1.f,0.f,0.f}};
};
struct LegacySpatialListener { float x{},y{},z{}; LegacySpatialBasis basis{}; };

struct LegacyAnglePair { float s{},c{1.f}; };
inline LegacyAnglePair legacyAnglePair(int angle){
    const int a=angle&0x7ff;
    const float r=float(a)*(3.14159265358979323846f/1024.0f);
    return {std::sin(r),std::cos(r)};
}
inline std::array<float,3> legacyRotateAudioVector(std::array<float,3> v,int ax,int ay,int az){
    const auto x=legacyAnglePair(ax), y=legacyAnglePair(ay), z=legacyAnglePair(az);
    const float t18=v[0]*z.s+v[1]*z.c;
    const float t8 =v[0]*z.c-v[1]*z.s;
    const float t4 =t18*y.s+v[2]*y.c;
    return {{t4*x.s+t8*x.c, t18*y.c-v[2]*y.s, t4*x.c-t8*x.s}};
}
inline LegacySpatialBasis legacyBuildAudioBasis(int ax,int ay,int az){
    LegacySpatialBasis out;
    out.a=legacyRotateAudioVector({{-1.f,0.f,0.f}},ax,ay,az);
    out.b=legacyRotateAudioVector({{ 1.f,0.f,0.f}},ax,ay,az);
    return out;
}

struct LegacySpatialGains { int left{},right{}; };

inline LegacySpatialGains legacySpatialGains(const LegacySpatialListener& l,float x,float y,float z,float scalar){
    float dx=x-l.x,dy=y-l.y,dz=z-l.z;
    const float d=std::sqrt(dx*dx+dy*dy+dz*dz);
    if(d>0.f){dx/=d;dy/=d;dz/=d;} else {dx=dy=dz=0.f;}
    const float base=(d>0.f)?scalar*LegacySpatialAudioTrace::combinedScale/d:float(LegacySpatialAudioTrace::channelClamp);
    const float da=dx*l.basis.a[0]+dy*l.basis.a[1]+dz*l.basis.a[2]+LegacySpatialAudioTrace::bootstrapBias;
    const float db=dx*l.basis.b[0]+dy*l.basis.b[1]+dz*l.basis.b[2]+LegacySpatialAudioTrace::bootstrapBias;
    // x87 FISTP obeys the current round-to-nearest mode in the original runtime.
    const int a=static_cast<int>(std::lrint(base*da));
    const int b=static_cast<int>(std::lrint(base*db));
    // 0x40A870/0x40A87B compare the signed FISTP result with 0x55 using
    // unsigned JBE. Normal game inputs are non-negative because the channel
    // dot is biased by +1.5, but preserving the unsigned cap also matches
    // the original behaviour for exceptional/invalid values.
    const auto cap=[](int v){
        return static_cast<unsigned int>(v)<=static_cast<unsigned int>(LegacySpatialAudioTrace::channelClamp)
            ? v : LegacySpatialAudioTrace::channelClamp;
    };
    return {cap(a),cap(b)};
}
} // namespace airxonix

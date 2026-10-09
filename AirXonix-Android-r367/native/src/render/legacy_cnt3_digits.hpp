#pragma once
#include "legacy_mesh.hpp"
#include <algorithm>
#include <array>
#include <vector>

namespace LegacyCnt3Digits {
constexpr float UStep=0.09843750298023224f;   // 0x43B2C0
constexpr float VTop=0.86328125f;             // 0x3F5D0000 = 220/256
constexpr float VBottom=0.9921875f;           // 0x3F7E0000 = 254/256
constexpr float Width=0.003000000026077032f;  // 0x41D5E8 caller arg
constexpr float Depth=0.004000000189989805f;  // 0x41D5B3 caller arg
constexpr float Light=0.80000001192092896f;   // 0x41D5AE caller arg

struct Mesh {
    std::vector<LegacyRenderVertex> vertices;
    std::vector<std::array<unsigned,4>> quads;
};

inline Mesh build(int value,int digitCount,float width=Width,float depth=Depth,float light=Light){
    digitCount=std::clamp(digitCount,0,6); // 0x40EE6F initializes six 0x60-byte digit slots.
    value=std::max(0,value);
    Mesh out;
    out.vertices.reserve(static_cast<std::size_t>(digitCount)*4u);
    out.quads.reserve(static_cast<std::size_t>(digitCount));
    int divisor=1;
    for(int i=1;i<digitCount;++i) divisor*=10;
    float x=0.f;
    for(int i=0;i<digitCount;++i){
        const int d=(value/divisor)%10;
        if(divisor>1) divisor/=10;
        const float u0=float(d)*UStep;
        const float u1=float(d+1)*UStep;
        const unsigned base=static_cast<unsigned>(out.vertices.size());
        out.vertices.push_back({x,0.f,0.f,u0,VBottom,light});
        out.vertices.push_back({x,0.f,depth,u0,VTop,light});
        x+=width;
        out.vertices.push_back({x,0.f,depth,u1,VTop,light});
        out.vertices.push_back({x,0.f,0.f,u1,VBottom,light});
        out.quads.push_back({base,base+1,base+2,base+3});
    }
    return out;
}
}

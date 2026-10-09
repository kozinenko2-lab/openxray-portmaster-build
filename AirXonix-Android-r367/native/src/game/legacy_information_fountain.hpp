#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include "core/legacy_random.hpp"

namespace LegacyInformationFountain {
inline constexpr int Count=128;
inline constexpr int Chunk=16;
inline constexpr int EmitThresholdMs=80;
inline constexpr float SpawnBaseX=0.04450000077486038f;
inline constexpr float SpawnOscX=0.007000000216066837f;
inline constexpr float SpawnY=-0.029999999329447746f;
inline constexpr float SpawnZ=0.06000000052154064f;
inline constexpr float BaseVy=0.00019999999494757503f;
inline constexpr float VyRandomScale=1.500000053056283e-6f;
inline constexpr float VxzRandomScale=2.0000000233721948e-7f;
inline constexpr float GravityPerMs=3.4999999343199306e-7f;

struct Particle { float x=0.f,y=0.f,z=0.f,vx=0.f,vy=0.f,vz=0.f; };

inline float spawnX(float sway){
    // 0x414530 stores -sway; 0x41463F takes sin and 0x414652/658 performs
    // 0.0445 - sin(-sway)*0.007.
    return SpawnBaseX + std::sin(sway)*SpawnOscX;
}

inline void emitChunk(std::array<Particle,Count>& pool,int& head,float sway,LegacyRandom& rng){
    head=(head+Chunk)&0x7f;
    const float x=spawnX(sway);
    for(int j=0;j<Chunk;++j){
        auto& p=pool[static_cast<std::size_t>(head+j)];
        p.x=x; p.y=SpawnY; p.z=SpawnZ;
        // DIRECT EXE call order 0x414674, 0x414694, 0x4146AE: vy, vx, vz.
        p.vy=BaseVy+float((rng.next()&0x3f)-0x20)*VyRandomScale;
        p.vx=float((rng.next()&0x3f)-0x0f)*VxzRandomScale;
        p.vz=float((rng.next()&0x3f)-0x20)*VxzRandomScale;
    }
}

inline void updateOne(Particle& p,int dtMs){
    const float dt=float(dtMs<0?0:dtMs);
    const float oldVy=p.vy;
    p.x+=p.vx*dt;
    p.y+=oldVy*dt;
    p.z+=p.vz*dt;
    p.vy=oldVy-GravityPerMs*dt;
    // 0x414755..0x414763: when the sign bit is set, only the low byte of
    // the newly stored vy float is zeroed, not the whole velocity/record.
    if(p.vy<0.f){
        std::uint32_t bits=0; std::memcpy(&bits,&p.vy,sizeof(bits));
        bits&=0xffffff00u; std::memcpy(&p.vy,&bits,sizeof(bits));
    }
}

inline void updateAll(std::array<Particle,Count>& pool,int dtMs){
    for(auto& p:pool) updateOne(p,dtMs);
}
}

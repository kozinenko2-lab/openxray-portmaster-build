#pragma once
#include <array>
#include <cstdint>
#include <cmath>
#include "core/legacy_random.hpp"

namespace LegacyParticles {
struct Velocity { float x=0.f,y=0.f,z=0.f; };

// 0x418040 / 0x418110 pickup-destruction burst.
constexpr int PickupCount=128;
constexpr int PickupLifetimeMs=2500;
constexpr float PickupGravityPerMs=1.0e-7f;
constexpr float PickupVelocityXZScale=4.0e-7f;
constexpr float PickupVelocityYScale=1.0e-6f;
constexpr float PickupU0=0.939453125f;
constexpr float PickupV0=0.126953125f;
constexpr float PickupUvSpan=0.05859375f;
constexpr std::array<std::uint32_t,6> PickupPackedColor{{
    0x00FFFF8Fu,0x008F8FFFu,0x00FF5F5Fu,0x00FFFFFFu,0x008FFF8Fu,0x00FF8FFFu
}};
inline Velocity pickupVelocity(LegacyRandom& rng){
    return {
        float((rng.next()&0xff)-128)*PickupVelocityXZScale,
        float(rng.next()&0x7f)*PickupVelocityYScale,
        float((rng.next()&0xff)-128)*PickupVelocityXZScale
    };
}

// 0x417390 / 0x417470 death burst used by crawler/player contact and other
// lethal world events. The EXE allocates one 512-record array, gives it a
// single 2500-ms lifetime, and renders it through the same pretransformed
// 0x40E120 billboard path.
constexpr int DeathCount=512;
constexpr int DeathLifetimeMs=2500;
constexpr float DeathRadialScale=9.0e-7f;
constexpr float DeathVerticalScale=2.0e-7f;
constexpr float DeathVerticalBias=1.5e-4f;
constexpr float DeathGravityPerMs=3.5e-7f;
constexpr float DeathBillboardU0=0.939453125f;
constexpr float DeathBillboardV0=0.126953125f;
constexpr float DeathBillboardUvSpan=0.05859375f;
constexpr float DeathBillboardSizeScale=0.000625f;
constexpr std::uint32_t DeathPackedColor=0x00FF8F8Fu;
inline Velocity deathVelocity(LegacyRandom& rng){
    const float radial=float((rng.next()&0xff)-128)*DeathRadialScale;
    const float angle=float((rng.next()&0xff)*2)*3.14159265358979323846f*(1.0f/255.0f);
    const float vy=float(rng.next()&0x7f)*DeathVerticalScale+DeathVerticalBias;
    return {std::cos(angle)*radial,vy,std::sin(angle)*radial};
}


// 0x418200 / 0x418290 death-scene tri-colour array.
// DIRECT EXE: one contiguous 480-record x/y/z/vx/vy/vz array, split into
// three 160-record render windows. r49 proved this belongs exclusively to
// death routine 0x41C030. r92 fresh disassembly closes 0x418290 too: all 480
// records use old-vy Euler integration, then vy -= 1.5e-7 * dt.
constexpr int DeathTriColorCount=480;
constexpr int DeathTriColorGroupCount=3;
constexpr int DeathTriColorGroupSize=160;
constexpr float DeathTriColorVelocityXZScale=4.0e-7f;
constexpr float DeathTriColorVelocityYScale=5.0e-7f;
constexpr float DeathTriColorGravityPerMs=1.5e-7f;
constexpr float DeathTriColorBillboardU0=0.939453125f;
constexpr float DeathTriColorBillboardV0=0.126953125f;
constexpr float DeathTriColorBillboardUvSpan=0.05859375f;
constexpr float DeathTriColorBillboardSizeScale=0.000531250028871f;
constexpr std::array<std::uint32_t,3> DeathTriColorPackedColor{{
    0x005FDF5Fu,0x00FF7F7Fu,0x00DFDFDFu
}};
inline Velocity deathTriColorVelocity(LegacyRandom& rng){
    return {
        float((rng.next()&0xff)-128)*DeathTriColorVelocityXZScale,
        float(rng.next()&0x7f)*DeathTriColorVelocityYScale,
        float((rng.next()&0xff)-128)*DeathTriColorVelocityXZScale
    };
}
inline void updateDeathTriColor(float& x,float& y,float& z,float& vy,float vx,float vz,int dtMs){
    const float dt=float(dtMs);
    const float oldVy=vy;
    x+=vx*dt;
    y+=oldVy*dt;
    z+=vz*dt;
    vy=oldVy-DeathTriColorGravityPerMs*dt;
}

// 0x4175B0 / 0x417700 field erosion debris.
constexpr int FieldPoolCount=96;
constexpr int FieldEmitMax=16;
constexpr float FieldSpawnY=0.006f;
constexpr float FieldFreeY=0.005f;
constexpr float FieldDeadY=-0.5f;
constexpr float FieldBillboardU0=0.939453125f;
constexpr float FieldBillboardV0=0.126953125f;
constexpr float FieldBillboardUvSpan=0.05859375f;
constexpr std::uint32_t FieldPackedColor=0x00CFAF4Fu;
constexpr float FieldBillboardSizeScale=0.000531250028871f;
constexpr float FieldVelocityXZScale=2.0e-7f;
constexpr float FieldVelocityYScale=5.0e-7f;
constexpr float FieldVelocityYBias=7.0e-5f;
constexpr float FieldGravityPerMs=4.0e-7f;
inline Velocity fieldVelocity(LegacyRandom& rng){
    return {
        float((rng.next()&0xff)-128)*FieldVelocityXZScale,
        float(rng.next()&0x7f)*FieldVelocityYScale+FieldVelocityYBias,
        float((rng.next()&0xff)-128)*FieldVelocityXZScale
    };
}
}

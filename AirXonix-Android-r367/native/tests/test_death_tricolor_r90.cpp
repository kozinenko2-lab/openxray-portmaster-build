#include "game/legacy_particles.hpp"
#include "core/legacy_random.hpp"
#include <array>
#include <cassert>
#include <cmath>

static bool nearf(float a,float b){ return std::fabs(a-b)<1e-12f; }

int main(){
    using namespace LegacyParticles;
    static_assert(DeathTriColorCount==480);
    static_assert(DeathTriColorGroupCount==3);
    static_assert(DeathTriColorGroupSize==160);
    static_assert(DeathTriColorGroupCount*DeathTriColorGroupSize==DeathTriColorCount);
    static_assert(DeathTriColorPackedColor[0]==0x005FDF5Fu);
    static_assert(DeathTriColorPackedColor[1]==0x00FF7F7Fu);
    static_assert(DeathTriColorPackedColor[2]==0x00DFDFDFu);

    LegacyRandom a,b;
    const auto v=deathTriColorVelocity(a);
    const float ex=float((b.next()&0xff)-128)*4.0e-7f;
    const float ey=float(b.next()&0x7f)*5.0e-7f;
    const float ez=float((b.next()&0xff)-128)*4.0e-7f;
    assert(nearf(v.x,ex)); assert(nearf(v.y,ey)); assert(nearf(v.z,ez));

    float x=1.f,y=2.f,z=3.f,vy=0.004f;
    const float vx=0.002f,vz=-0.001f;
    updateDeathTriColor(x,y,z,vy,vx,vz,10);
    assert(std::fabs(x-1.02f)<1e-6f);
    assert(std::fabs(y-2.04f)<1e-6f); // old vy is used for Y integration
    assert(std::fabs(z-2.99f)<1e-6f);
    assert(std::fabs(vy-(0.004f-10.f*DeathTriColorGravityPerMs))<1e-9f);
    static_assert(DeathTriColorBillboardU0==0.939453125f);
    static_assert(DeathTriColorBillboardV0==0.126953125f);
    static_assert(DeathTriColorBillboardUvSpan==0.05859375f);
    return 0;
}

#include "game/legacy_particles.hpp"
#include <cassert>
#include <cmath>
int main(){
    static_assert(LegacyParticles::PickupCount==128);
    static_assert(LegacyParticles::PickupLifetimeMs==2500);
    static_assert(LegacyParticles::FieldPoolCount==96);
    static_assert(LegacyParticles::FieldEmitMax==16);
    LegacyRandom a(1u),b(1u);
    const auto v=LegacyParticles::pickupVelocity(a);
    const int r0=b.next(),r1=b.next(),r2=b.next();
    assert(std::fabs(v.x-float((r0&255)-128)*4e-7f)<1e-12f);
    assert(std::fabs(v.y-float(r1&127)*1e-6f)<1e-12f);
    assert(std::fabs(v.z-float((r2&255)-128)*4e-7f)<1e-12f);
    assert(a.state()==b.state());
    assert(LegacyParticles::PickupPackedColor[2]==0x00FF5F5Fu);
    return 0;
}

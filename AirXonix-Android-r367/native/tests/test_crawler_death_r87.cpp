#include "game/entities.hpp"
#include "game/legacy_particles.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

struct EntitiesTestProbe {
    static std::vector<GroundEnemy>& ground(Entities& e){ return e.ground_; }
};

static bool nearf(float a,float b,float eps=1e-8f){return std::fabs(a-b)<=eps;}

int main(){
    Entities e;
    auto& g=EntitiesTestProbe::ground(e);
    g.resize(1);
    g[0].active=true;
    g[0].x=20.f; g[0].y=30.f;
    g[0].worldX=.5f; g[0].worldZ=.5f;
    g[0].respawnDelay=.014f;
    float hx=0.f,hz=0.f;

    // 0x409140 threshold is strict distance^2 < 4.225e-5 (= .0065^2).
    assert(e.consumeCrawlerPlayerHit(.5064f,.5f,hx,hz));
    assert(nearf(hx,.5f) && nearf(hz,.5f));
    assert(nearf(g[0].x,32.f));
    assert(nearf(g[0].y,65.f));
    assert(nearf(g[0].respawnDelay,1.7f));

    g[0].worldX=.5f; g[0].worldZ=.5f; g[0].respawnDelay=.014f;
    assert(!e.consumeCrawlerPlayerHit(.5065f,.5f,hx,hz));
    g[0].respawnDelay=.015f;
    assert(!e.consumeCrawlerPlayerHit(.5f,.5f,hx,hz));

    // Exact 0x417390 RNG formula: compare helper to the same first three
    // MSVC rand() values from an independently seeded stream.
    LegacyRandom a(1u), b(1u);
    const auto v=LegacyParticles::deathVelocity(a);
    const int r0=b.next()&0xff;
    const int r1=b.next()&0xff;
    const int r2=b.next()&0x7f;
    const float radial=float(r0-128)*LegacyParticles::DeathRadialScale;
    const float angle=float(r1*2)*3.14159265358979323846f/255.f;
    assert(nearf(v.x,std::cos(angle)*radial,1e-10f));
    assert(nearf(v.y,float(r2)*LegacyParticles::DeathVerticalScale+LegacyParticles::DeathVerticalBias,1e-10f));
    assert(nearf(v.z,std::sin(angle)*radial,1e-10f));
    assert(LegacyParticles::DeathCount==512);
    assert(LegacyParticles::DeathLifetimeMs==2500);
    assert(nearf(LegacyParticles::DeathGravityPerMs,3.5e-7f,1e-12f));
    std::cout << "crawler death r87 ok\n";
}

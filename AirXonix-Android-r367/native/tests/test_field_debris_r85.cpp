#include "game/entities.hpp"
#include "game/legacy_particles.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

struct EntitiesTestProbe {
    static void emit(Entities& e,int x,int y,LegacyRandom& rng){ e.emitFieldDebris(x,y,rng); }
    static void step(Entities& e,int dt){ e.updateFieldDebris(dt); }
};

static bool nearf(float a,float b,float eps=1e-9f){ return std::fabs(a-b)<=eps; }

int main(){
    Entities e;
    LegacyRandom rng(1u);
    EntitiesTestProbe::emit(e,10,20,rng);
    const auto& p=e.fieldDebris();
    int active=0;
    for(const auto& q:p) if(q.active()) ++active;
    assert(active==16);
    const float expectedX=0.4f+10.f*0.003125f;
    const float expectedZ=0.4f+20.f*0.003125f;
    assert(nearf(p[0].x,expectedX,1e-7f));
    assert(nearf(p[0].y,LegacyParticles::FieldSpawnY,1e-9f));
    assert(nearf(p[0].z,expectedZ,1e-7f));

    const float oldVy=p[0].vy;
    const float oldX=p[0].x, oldY=p[0].y, oldZ=p[0].z;
    const float vx=p[0].vx, vz=p[0].vz;
    EntitiesTestProbe::step(e,10);
    const auto& q=e.fieldDebris()[0];
    const float vy2=oldVy-10.f*LegacyParticles::FieldGravityPerMs;
    assert(nearf(q.vy,vy2,1e-9f));
    assert(nearf(q.x,oldX+vx*10.f,1e-7f));
    // r195 direct re-check: position integrates the OLD vertical velocity;
    // gravity updates vy only after that frame's position step.
    assert(nearf(q.y,oldY+oldVy*10.f,1e-7f));
    assert(nearf(q.z,oldZ+vz*10.f,1e-7f));

    // Repeated emit calls can never exceed the original shared 96-slot pool.
    for(int i=0;i<10;++i) EntitiesTestProbe::emit(e,11+i,21,rng);
    active=0; for(const auto& r:e.fieldDebris()) if(r.active()) ++active;
    assert(active<=LegacyParticles::FieldPoolCount);
    assert(active==LegacyParticles::FieldPoolCount);

    std::cout << "field debris r85 ok\n";
}

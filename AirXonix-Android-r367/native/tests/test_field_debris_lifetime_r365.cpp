#include "game/entities.hpp"
#include "game/legacy_particles.hpp"
#include <cmath>
#include <iostream>

struct EntitiesTestProbeR365 {
    static FieldDebrisParticle& at(Entities& e,std::size_t i){return e.fieldDebris_[i];}
    static void step(Entities& e,int dt){e.updateFieldDebris(dt);}
};
static bool near(float a,float b,float eps=1e-8f){return std::fabs(a-b)<=eps;}
static bool check(bool v,const char* m){if(!v)std::cerr<<"FAIL: "<<m<<"\n";return v;}
int main(){
    bool ok=true;
    Entities e; Field f; LevelRecord empty{}; f.build(empty); LegacyRandom rng(1u);
    // Seed process-lifetime data on both sides of the native 64/32 split.
    auto& a=EntitiesTestProbeR365::at(e,5); a={.11f,.22f,.33f,.44f,.55f,.66f};
    auto& b=EntitiesTestProbeR365::at(e,80); b={.21f,.31f,.41f,.51f,.61f,.71f};
    e.reset(empty,f,rng);
    const auto& aa=e.fieldDebris()[5];
    const auto& bb=e.fieldDebris()[80];
    ok &= check(near(aa.y,-1.f),"level init must set Y=-1 for first 64 debris records");
    ok &= check(near(aa.x,.11f)&&near(aa.z,.33f)&&near(aa.vx,.44f)&&near(aa.vy,.55f),
                "level init must preserve non-Y fields in first 64 records");
    ok &= check(near(bb.x,.21f)&&near(bb.y,.31f)&&near(bb.z,.41f)&&near(bb.vy,.61f),
                "last 32 debris records must persist untouched across level init");

    // Emitter-free (Y<=.005) does not mean physics-inactive. Native 0x417700
    // integrates any non-negative record, including BSS-zero/free entries.
    auto& z=EntitiesTestProbeR365::at(e,70); z={1.f,0.f,2.f,.1f,0.f,.2f};
    ok &= check(!z.active(),"Y=0 must be emitter-free");
    EntitiesTestProbeR365::step(e,10);
    ok &= check(near(z.x,2.f)&&near(z.z,4.f),"Y=0 record must still integrate X/Z in 0x417700");
    ok &= check(near(z.vy,-10.f*LegacyParticles::FieldGravityPerMs,1e-9f),
                "Y=0 record must still receive gravity");

    if(!ok)return 1;
    std::cout<<"field debris lifetime r365 ok\n";
}

#include "game/entities.hpp"
#include <cmath>
#include <iostream>

struct EntitiesTestProbeR363 {
    static void setCached(Entities& e,std::size_t i,float x,float z){
        e.air_[i].worldX=x; e.air_[i].worldZ=z;
    }
};

static bool near(float a,float b){return std::fabs(a-b)<1.0e-7f;}
static bool check(bool v,const char* msg){if(!v)std::cerr<<"FAIL: "<<msg<<"\n";return v;}

int main(){
    bool ok=true;
    Field f; LevelRecord empty{}; f.build(empty);

    // Fresh process/BSS: 0x415BA0 never writes +0x20/+0x24.
    LevelRecord one{}; one.enemySpeed=8; one.enemyTypeACount=1;
    LegacyRandom rng1(1u); Entities e;
    e.reset(one,f,rng1);
    ok &= check(e.air().size()==1,"one airborne record expected");
    ok &= check(near(e.air()[0].worldX,0.f)&&near(e.air()[0].worldZ,0.f),
                "fresh airborne cached world coordinates must retain BSS zero");

    // Reused slot keeps its previous cached values; a newly-used second slot is
    // still BSS zero. This is the exact static-array lifetime of 0x257E3D0.
    EntitiesTestProbeR363::setCached(e,0,0.5125f,0.43125f);
    LevelRecord two{}; two.enemySpeed=8; two.enemyTypeACount=2;
    LegacyRandom rng2(77u);
    e.reset(two,f,rng2);
    ok &= check(e.air().size()==2,"two airborne records expected");
    ok &= check(near(e.air()[0].worldX,0.5125f)&&near(e.air()[0].worldZ,0.43125f),
                "reused airborne slot must preserve +0x20/+0x24 across level init");
    ok &= check(near(e.air()[1].worldX,0.f)&&near(e.air()[1].worldZ,0.f),
                "newly-used airborne slot must retain BSS-zero cache");

    if(!ok)return 1;
    std::cout<<"air cached world persistence r363 ok\n";
    return 0;
}

#include "game/special_objects.hpp"
#include <cassert>
#include <iostream>

struct SpecialObjectsTestProbe2 {
    static HomingSpecial& homing(SpecialObjects& s){ return s.homing_; }
};

int main(){
    SpecialObjects s;
    auto& h=SpecialObjectsTestProbe2::homing(s);
    h.active=true; h.speed=.000002f; h.worldX=.5f; h.worldZ=.5f;
    float x=0,y=0,z=0;
    h.height=.025f;
    assert(!s.consumeHomingPlayerHit(.5f,.5f,x,y,z));
    h.height=.024f;
    assert(!s.consumeHomingPlayerHit(.5101f,.5f,x,y,z));
    assert(s.consumeHomingPlayerHit(.5099f,.5f,x,y,z));
    assert(x==.5f && y==.024f && z==.5f);
    assert(h.worldX==.5f && h.height==.25f && h.worldZ==.6f);
    std::cout << "homing death r89 ok\n";
}

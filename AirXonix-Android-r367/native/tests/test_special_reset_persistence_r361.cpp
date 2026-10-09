#include "game/special_objects.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

struct SpecialObjectsTestProbe {
    static HomingSpecial& homing(SpecialObjects& s){ return s.homing_; }
    static EraserSpecial& eraser(SpecialObjects& s){ return s.eraser_; }
    static int& debrisHead(SpecialObjects& s){ return s.eraserDebrisHead_; }
    static bool& redirect(SpecialObjects& s){ return s.externalRedirect_; }
    static std::array<EraserDebrisParticle,128>& debris(SpecialObjects& s){ return s.eraserDebris_; }
};

static bool near(float a,float b,float e=1e-7f){ return std::fabs(a-b)<=e; }

int main(){
    SpecialObjects s;
    auto& h=SpecialObjectsTestProbe::homing(s);
    h.spinPhase=1.25f;
    h.soundPhase=2.5f;
    h.directionX=.3f;
    h.directionZ=-.4f;
    h.retargetClock=0x1234;

    auto& e=SpecialObjectsTestProbe::eraser(s);
    e.spinPhase=3.25f;
    e.rotationAngle2048=0x456;
    SpecialObjectsTestProbe::debrisHead(s)=64;
    SpecialObjectsTestProbe::redirect(s)=true;
    SpecialObjectsTestProbe::debris(s)[7].active=true;
    SpecialObjectsTestProbe::debris(s)[7].y=.123f;

    LevelRecord level{};
    level.specialHoming=7;
    level.specialEraser=9;
    LegacyRandom rng(1);
    s.reset(level,rng);

    // 0x415440 does not touch C4/C8/CC/D0/D4.
    assert(near(h.spinPhase,1.25f));
    assert(near(h.soundPhase,2.5f));
    assert(near(h.directionX,.3f));
    assert(near(h.directionZ,-.4f));
    assert(h.retargetClock==0x1234);
    assert(near(h.worldX,.5f) && near(h.height,.25f) && near(h.worldZ,.5f));
    assert(near(h.speed,7.f*.000002f));

    // 0x414DD0 does not touch A0/A4/A8/AC, but does clear all 128 debris records.
    assert(near(e.spinPhase,3.25f));
    assert(e.rotationAngle2048==0x456);
    assert(SpecialObjectsTestProbe::debrisHead(s)==64);
    assert(SpecialObjectsTestProbe::redirect(s));
    assert(!SpecialObjectsTestProbe::debris(s)[7].active);
    assert(near(SpecialObjectsTestProbe::debris(s)[7].y,0.f));

    std::cout << "special reset persistence r361 ok\n";
}

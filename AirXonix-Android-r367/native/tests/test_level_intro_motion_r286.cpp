#include "game/game.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
static bool nearf(float a,float b,float e=2e-7f){return std::fabs(a-b)<=e;}
int main(){
    Game g;
    InputState in{};
    auto s=g.levelIntroScene();
    assert(nearf(s.plaqueY,-.1f));
    assert(nearf(s.plaqueAngleRad,0.f));
    assert(nearf(s.lightByte,0.f));
    assert(nearf(s.presentedLightByte,0.f));
    assert(nearf(s.plaqueX(1),-.0022f));
    assert(nearf(s.digitX(1),.0058f));
    assert(nearf(s.plaqueX(10),-.003f));
    assert(nearf(s.digitX(10),.005f));
    g.update(in,600); // remaining 2400: cue14 and approach phase
    s=g.levelIntroScene();
    assert(s.cue14Consumed && !s.cue5Consumed);
    assert(nearf(s.presentedLightByte,0.f));
    assert(nearf(s.lightByte,81.f,2e-5f));
    assert(nearf(s.plaqueY,-.04f));
    assert(nearf(s.plaqueRotationRad(),0.f));
    g.update(in,1000); // remaining 1400, clamps at -.025
    s=g.levelIntroScene();
    assert(nearf(s.plaqueY,-.025f));
    g.update(in,500); // remaining 900: late branch owns full dt
    s=g.levelIntroScene();
    assert(s.cue5Consumed);
    assert(nearf(s.plaqueY,-.055f));
    // r347: 0x41D5CC / fallback 0x41DC1C initialize the independent
    // LEV2 angle to 0. The -0.1 constant belongs to plaque Y.
    assert(nearf(s.plaqueRotationRad(),500.f*0.01f,2e-6f));
    assert(nearf(s.digitY(),-.025f));
    assert(nearf(s.digitLateX,.01f));
    std::cout << "level intro motion r286 PASS\n";
}

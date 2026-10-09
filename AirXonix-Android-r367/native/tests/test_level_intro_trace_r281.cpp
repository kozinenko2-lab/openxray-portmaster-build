#include "game/legacy_level_intro_trace.hpp"
#include <cassert>
#include <iostream>
int main(){
    using namespace LegacyLevelIntro;
    assert(kTrace.durationMs==3000);
    assert(kTrace.overlayMotionThresholdMs==1000);
    assert(kTrace.initialCameraAngle2==-450);
    assert(kTrace.initialPlaqueAngleRad==0.f);
    assert(kTrace.lightMin==0.f && kTrace.lightMax==255.f);
    assert(kTrace.lightVelocityPerMs>0.134f && kTrace.lightVelocityPerMs<0.136f);
    assert(kTrace.lightSeed==0x0041D50Eu && kTrace.lightExit==0x0041D673u);
    assert(kTrace.lev2Submit==0x0041DB0Au);
    assert(kTrace.cnt3Submit==0x0041DB58u);
    assert(kTrace.rendersWorldBeforeOverlay && kTrace.usesLev2 && kTrace.usesCnt3Digits);
    std::cout << "level intro trace r281 PASS\n";
}

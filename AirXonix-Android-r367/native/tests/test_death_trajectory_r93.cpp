#include "core/legacy_highscore.hpp"
#define private public
#include "game/game.hpp"
#undef private
#include <cassert>
#include <cmath>
#include <iostream>

static bool nearf(float a,float b,float eps=1e-6f){return std::fabs(a-b)<=eps;}

int main(){
    Game g;
    g.lives_=2;
    g.player_.reset();
    g.handleDeath();
    assert(g.phase_==GamePhase::Dying);
    assert(nearf(g.deathScene_.worldY,0.008f));
    assert(nearf(g.deathScene_.velocityY,0.00018f,1e-8f));
    assert(!g.deathScene_.triColorBurstStarted);

    // One literal Euler frame from 0x41C2B7..0x41C398.
    g.updateDeathSequence(10);
    assert(nearf(g.deathScene_.worldY,0.0098f,2e-6f));
    assert(nearf(g.deathScene_.velocityY,0.0001765f,2e-8f));

    // Advance in small frame-like slices until the Y<.008 transition fires.
    for(int i=0;i<300 && !g.deathScene_.triColorBurstStarted;++i)
        g.updateDeathSequence(10);
    assert(g.deathScene_.triColorBurstStarted);
    assert(g.deathTriColorInitialized_);
    assert(nearf(g.deathScene_.worldY,0.5f,1e-7f));
    assert(nearf(g.brightnessScale_,0.55f,1e-7f));
    assert(g.deathScene_.cameraShakeAmplitude<13.0f);
    assert(g.displayCameraYawOffset()==g.deathScene_.cameraYawOffset);

    // With one life remaining, the <1000ms branch places Xonix at .1 and
    // descends at 9e-5/ms toward .008.
    while(g.deathScene_.remainingMs()>=1000) g.updateDeathSequence(10);
    assert(g.lives_==1);
    assert(g.deathScene_.finalSecondRespawnStarted);
    assert(g.deathScene_.worldY<=0.1f && g.deathScene_.worldY>=0.008f);

    std::cout << "death trajectory r93 ok\n";
}

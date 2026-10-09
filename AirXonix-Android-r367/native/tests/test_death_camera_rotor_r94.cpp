#include "core/legacy_highscore.hpp"
#define private public
#include "game/game.hpp"
#undef private
#include "render/legacy_camera.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

static bool nearf(float a,float b,float eps=1e-6f){return std::fabs(a-b)<=eps;}

int main(){
    // Direct camera equations from 0x41C370 / 0x41C7D9.
    auto pre=LegacyCamera::deathState(0.5f,0.108f,0.5f,false,0);
    assert(nearf(pre.x,LegacyCamera::trackedX(0.5f)));
    assert(nearf(pre.y,0.153f,2e-6f));
    assert(nearf(pre.z,LegacyCamera::trackedZ(0.5f)));
    auto post=LegacyCamera::deathState(0.5f,0.5f,0.5f,true,7);
    assert(nearf(post.y,0.103f));
    assert(post.angle3==7);

    Game g;
    g.lives_=2;
    g.playerRotorPhase_=1.0f;
    g.handleDeath();
    assert(nearf(g.deathScene_.rotorPhase,1.0f));
    assert(nearf(g.deathScene_.rotorRadius,0.0057f));
    g.updateDeathSequence(10);
    assert(nearf(g.deathScene_.rotorPhase,1.07f,2e-6f));
    assert(nearf(g.displayPlayerRotorRadius(),0.0057f));

    // Force the post-burst centre-directed path to overshoot in one frame.
    g.deathScene_.triColorBurstStarted=true;
    g.deathTriColorInitialized_=true;
    g.deathScene_.elapsedMs=1000;
    g.deathScene_.worldX=0.50f;
    g.deathScene_.worldZ=0.40f;
    g.deathScene_.velocityX=0.001f;
    g.deathScene_.velocityZ=0.001f;
    g.updateDeathSequence(10);
    assert(nearf(g.deathScene_.worldX,0.5015625358f,2e-7f));
    assert(nearf(g.deathScene_.worldZ,0.4015625119f,2e-7f));

    // Normal final-second respawn retracts the rotor to the original .0047.
    g.deathScene_.elapsedMs=3010;
    g.updateDeathSequence(10);
    assert(g.deathScene_.remainingMs()<1000);
    assert(nearf(g.deathScene_.rotorRadius,0.0047f,1e-7f));

    // Zero-lives branch: EXE local starts at -.3, producing grayscale 255,
    // then tends toward -.03 => about 174.
    Game z;
    z.lives_=1;
    z.handleDeath(); // leaves zero lives but stays in Dying
    z.deathScene_.triColorBurstStarted=true;
    z.deathTriColorInitialized_=true;
    z.deathScene_.elapsedMs=3001;
    z.updateDeathSequence(1);
    assert(z.deathScene_.zeroLivesGray<=255 && z.deathScene_.zeroLivesGray>=174);
    for(int i=0;i<500 && z.phase_==GamePhase::Dying;++i) z.updateDeathSequence(1);
    assert(z.deathScene_.zeroLivesGray>=174);

    std::cout << "death camera/rotor r94 ok\n";
}

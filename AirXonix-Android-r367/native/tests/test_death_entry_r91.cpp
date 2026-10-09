#include "core/legacy_highscore.hpp"
#define private public
#include "game/game.hpp"
#undef private
#include <cassert>
#include <cmath>
#include <iostream>

static bool nearf(float a,float b){ return std::fabs(a-b)<1.0e-7f; }

int main(){
    Game g;
    // Simulate a previous blackout/high-numbered pickup state: death entry must
    // overwrite it with the literal 0x41C030 value 0.8.
    g.brightnessScale_=0.0f;
    g.lives_=3;
    g.handleDeath();
    assert(g.phase_==GamePhase::Dying);
    assert(g.deathScene_.durationMs==4000);
    assert(g.deathScene_.elapsedMs==0);
    assert(nearf(g.brightnessScale_,0.8f));
    std::cout << "death entry r91 ok\n";
}

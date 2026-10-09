#include "core/legacy_highscore.hpp"
#define private public
#include "game/game.hpp"
#undef private
#include <cassert>
#include <cmath>
#include <iostream>

int main(){
    Game g;
    g.lives_=1;
    g.handleDeath();
    assert(g.lives_==0);
    assert(g.phase_==GamePhase::Dying);
    assert(g.deathScene_.remainingMs()==4000);
    g.updateDeathSequence(2999);
    assert(g.phase_==GamePhase::Dying);
    assert(!g.deathScene_.lastSecond());
    g.updateDeathSequence(2);
    assert(g.phase_==GamePhase::Dying);
    assert(g.deathScene_.lastSecond());
    g.updateDeathSequence(999);
    // DIRECT EXE 0x41C625: signed remaining must be strictly < 0.
    assert(g.phase_==GamePhase::Dying);
    assert(g.deathScene_.remainingMs()==0);
    g.updateDeathSequence(1);
    assert(g.phase_==GamePhase::GameOver);
    std::cout << "death last-life r92 ok\n";
}

#include "game/game.hpp"
#include <cassert>
#include <iostream>
int main(){
    Game g;
    assert(g.phase()==GamePhase::Gameplay);
    assert(g.levelIntroActive());
    const int t0=g.timeRemaining();
    InputState in{}; in.right=true; in.pause=true; in.select=true;
    g.update(in,1000);
    assert(g.levelIntroActive());
    assert(g.timeRemaining()==t0);
    g.update(in,1000);
    assert(g.levelIntroActive());
    assert(g.timeRemaining()==t0);
    g.update(in,1000);
    // r347: EBP has reached zero, but capable 0x41D4D0 remains alive until
    // the 0..255..0 light pulse crosses below zero.
    assert(g.levelIntroActive());
    assert(!g.levelEntryActive());
    g.update(in,1000);
    assert(!g.levelIntroActive());
    assert(g.levelEntryActive());
    assert(g.timeRemaining()==t0);
    InputState neutral{};
    // r295: 0x41CEA0 continues with the live-world light/camera entrance before gameplay.
    for(int i=0;i<20 && g.levelEntryActive();++i)g.update(neutral,100);
    assert(!g.levelEntryActive());
    assert(g.timeRemaining()==t0);
    g.update(neutral,16);
    assert(g.timeRemaining()==t0-16);
    std::cout << "level intro runtime r284 PASS\n";
}

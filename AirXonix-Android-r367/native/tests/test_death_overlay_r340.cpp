#include "core/legacy_highscore.hpp"
#define private public
#include "game/game.hpp"
#undef private
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
 Game g;
 g.startDeathOverlay(.5f,.025f,.4f,255.f,50.f,0.f);
 assert(g.deathOverlay_.active);
 assert(std::fabs(g.deathOverlay_.halfExtent-.01f)<1e-6f);
 g.updateDeathOverlay(10);
 assert(std::fabs(g.deathOverlay_.intensity-1.f)<1e-6f);
 assert(g.deathOverlay_.halfExtent>.0114f);
 g.updateDeathOverlay(10);
 assert(std::fabs(g.deathOverlay_.intensity-.97f)<1e-5f);
 for(int i=0;i<40 && g.deathOverlay_.active;++i)g.updateDeathOverlay(10);
 assert(!g.deathOverlay_.active);
 std::cout<<"death overlay r340 PASS\n";
}

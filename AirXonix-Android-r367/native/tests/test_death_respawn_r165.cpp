#include "game/player.hpp"
#include <cassert>
#include <cmath>
int main(){
    Player p; p.resetAfterDeath();
    assert(p.x()==32 && p.y()==0 && p.prevX()==32 && p.prevY()==0);
    assert(std::fabs(p.worldX()-0.5015625357627869f)<1e-7f);
    assert(std::fabs(p.worldZ()-0.40156251192092896f)<1e-7f);
    assert(std::fabs(p.visualY()-0.00800000037997961f)<1e-8f);
    return 0;
}

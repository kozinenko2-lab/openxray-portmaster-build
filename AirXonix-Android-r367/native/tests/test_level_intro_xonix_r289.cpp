#include "game/game.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
static bool nearf(float a,float b,float e=3e-6f){return std::fabs(a-b)<=e;}
int main(){
    Game g; InputState in{};
    const float r0=g.displayPlayerRotorPhase();
    const float p0=g.displayPlayerPropellerPhase();
    g.update(in,100);
    assert(nearf(g.displayPlayerRotorRadius(),0.0045f));
    assert(nearf(g.displayPlayerRotorPhase(),r0+0.5f));
    // min(100*.04,pi/2) is pi/2, subtracted and wrapped.
    float expect=p0-1.57079632679f; while(expect<0.f)expect+=6.28318530718f;
    assert(nearf(g.displayPlayerPropellerPhase(),expect));
    std::cout << "level intro xonix r289 PASS\n";
}

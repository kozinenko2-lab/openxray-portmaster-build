#include "game/game.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
static bool nearf(float a,float b,float e=3e-7f){return std::fabs(a-b)<=e;}
int main(){
    Game g; InputState in{};
    assert(g.levelIntroActive());
    assert(nearf(g.displayPlayerWorldX(),0.501562476f));
    assert(nearf(g.displayPlayerHeight(),0.32f));
    assert(nearf(g.levelIntroCameraY(),0.423f));
    assert(g.levelIntroCameraAngle2()==-450);
    g.update(in,500);
    assert(nearf(g.displayPlayerHeight(),0.29f));
    assert(nearf(g.levelIntroCameraY(),0.393f));
    assert(nearf(g.levelIntroCameraY()-g.displayPlayerHeight(),0.103f));
    std::cout << "level intro camera r288 PASS\n";
}

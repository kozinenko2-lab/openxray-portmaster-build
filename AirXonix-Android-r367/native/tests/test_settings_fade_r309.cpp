#include "game/game.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace airxonix;
struct GameTestProbe {
    static void enter(Game& g){ g.enterSettings(); }
    static void update(Game& g,const InputState& in,int dt){ g.updateSettings(in,dt); }
    static SettingsState& state(Game& g){ return g.settings_; }
};
int main(){
    static_assert(LegacySettingsTrace::fadeMax==0x7C0);
    static_assert(LegacySettingsTrace::fadeInRate==7);
    assert(std::fabs(LegacySettingsTrace::modelLightScale(0)-0.f)<1e-6f);
    assert(std::fabs(LegacySettingsTrace::modelLightScale(0x7C0)-(248.f/255.f))<1e-6f);
    Game g; GameTestProbe::enter(g);
    InputState in{}; in.down=true;
    GameTestProbe::update(g,in,100);
    assert(GameTestProbe::state(g).selected==0 && !GameTestProbe::state(g).readyForInput);
    in={}; GameTestProbe::update(g,in,200);
    assert(GameTestProbe::state(g).fadeCounter==0x7C0 && GameTestProbe::state(g).readyForInput);
    in.down=true; GameTestProbe::update(g,in,16);
    assert(GameTestProbe::state(g).selected==1);
    assert(GameTestProbe::state(g).selectorOffset<0.f);
    const float firstOffset=GameTestProbe::state(g).selectorOffset;
    // A second row event is blocked until the -0.0028 camera slide settles.
    in={}; in.down=true; GameTestProbe::update(g,in,16);
    assert(GameTestProbe::state(g).selected==1);
    assert(GameTestProbe::state(g).selectorOffset<firstOffset);
    in={}; for(int i=0;i<8;++i)GameTestProbe::update(g,in,16);
    assert(std::fabs(GameTestProbe::state(g).selectorOffset-LegacySettingsTrace::selectorTarget(1))<1e-7f);
    std::cout<<"settings fade r309 PASS\n";
}

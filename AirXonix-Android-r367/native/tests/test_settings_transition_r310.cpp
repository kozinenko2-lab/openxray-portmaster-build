#include "game/game.hpp"
#include <cassert>
#include <iostream>
using namespace airxonix;
struct GameTestProbe {
    static void enter(Game& g){g.enterSettings();}
    static void update(Game& g,const InputState& i,int dt){g.updateSettings(i,dt);}
    static SettingsState& s(Game& g){return g.settings_;}
    static void controls(Game& g,const InputState& i,int dt=16){g.updateControlsRemap(i,dt);}
};
static void arm(Game& g){
    InputState none{};
    for(int i=0;i<20 && !GameTestProbe::s(g).readyForInput;++i)GameTestProbe::update(g,none,32);
    assert(GameTestProbe::s(g).readyForInput);
}
int main(){
    static_assert(LegacySettingsTrace::fadeOutRate==-6);
    static_assert(LegacySettingsTrace::controlsReturnFadeInRate==6);
    Game g; GameTestProbe::enter(g); arm(g);
    GameTestProbe::s(g).selected=4; GameTestProbe::s(g).selectorOffset=LegacySettingsTrace::selectorTarget(4);
    InputState action{};action.action=true;GameTestProbe::update(g,action,16);
    assert(g.phase()==GamePhase::Settings);
    assert(GameTestProbe::s(g).pendingTransition==SettingsState::PendingTransition::Controls);
    InputState none{};
    for(int i=0;i<30 && g.phase()==GamePhase::Settings;++i)GameTestProbe::update(g,none,16);
    assert(g.phase()==GamePhase::Controls);
    while(!g.controlsRemap().readyForInput)GameTestProbe::controls(g,none,100);
    InputState back{};back.back=true;GameTestProbe::controls(g,back);
    assert(g.phase()==GamePhase::Controls);
    while(g.phase()==GamePhase::Controls)GameTestProbe::controls(g,none,100);
    assert(g.phase()==GamePhase::Settings);
    assert(GameTestProbe::s(g).fadeCounter==0);
    assert(GameTestProbe::s(g).fadeRate==6);
    assert(!GameTestProbe::s(g).readyForInput);
    std::cout<<"settings transition r310 PASS\n";
}

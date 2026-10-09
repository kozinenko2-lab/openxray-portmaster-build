#include "game/game.hpp"
#include <cassert>
#include <iostream>
using namespace airxonix;
struct GameTestProbe {
    static void enter(Game& g){g.enterSettings();}
    static void update(Game& g,const InputState& i,int dt){g.updateSettings(i,dt);}
    static SettingsState& s(Game& g){return g.settings_;}
};
static void arm(Game& g){InputState n{};for(int i=0;i<20&&!GameTestProbe::s(g).readyForInput;++i)GameTestProbe::update(g,n,32);}
static bool hasClick(const std::vector<DeathAudioEvent>& e){for(auto& q:e)if(q.kind==DeathAudioEventKind::SimplePlay&&q.logicalId==0x16u)return true;return false;}
int main(){
    Game g; GameTestProbe::enter(g); g.takeDeathAudioEvents(); arm(g); g.takeDeathAudioEvents();
    GameTestProbe::s(g).selected=3; GameTestProbe::s(g).selectorOffset=LegacySettingsTrace::selectorTarget(3);
    const bool a=GameTestProbe::s(g).speech;
    InputState l{};l.left=true;GameTestProbe::update(g,l,16);
    assert(GameTestProbe::s(g).speech!=a);assert(hasClick(g.takeDeathAudioEvents()));
    // held key is not a second queue event
    GameTestProbe::update(g,l,16);assert(GameTestProbe::s(g).speech!=a);
    InputState n{};GameTestProbe::update(g,n,16);
    InputState r{};r.right=true;GameTestProbe::update(g,r,16);
    assert(GameTestProbe::s(g).speech==a);assert(hasClick(g.takeDeathAudioEvents()));
    GameTestProbe::update(g,n,16);
    InputState act{};act.action=true;GameTestProbe::update(g,act,16);
    assert(GameTestProbe::s(g).speech!=a);assert(hasClick(g.takeDeathAudioEvents()));
    std::cout<<"settings speech r311 PASS\n";
}

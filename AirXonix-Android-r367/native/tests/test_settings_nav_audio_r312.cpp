#include "game/game.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace airxonix;
struct GameTestProbe { static void enter(Game& g){g.enterSettings();} static void update(Game& g,const InputState&i,int d){g.updateSettings(i,d);} static SettingsState& s(Game&g){return g.settings_;} };
static void arm(Game& g){InputState n{};for(int i=0;i<20&&!GameTestProbe::s(g).readyForInput;++i)GameTestProbe::update(g,n,32);g.takeDeathAudioEvents();}
static int count(const std::vector<DeathAudioEvent>& e,unsigned id){int n=0;for(auto&q:e)if(q.kind==DeathAudioEventKind::SimplePlay&&q.logicalId==id)++n;return n;}
int main(){Game g;GameTestProbe::enter(g);arm(g);InputState d{};d.down=true;GameTestProbe::update(g,d,16);assert(GameTestProbe::s(g).selected==1);assert(count(g.takeDeathAudioEvents(),0x15u)==1);InputState n{};for(int i=0;i<16 && std::fabs(GameTestProbe::s(g).selectorOffset-LegacySettingsTrace::selectorTarget(1))>1e-7f;++i)GameTestProbe::update(g,n,16);InputState u{};u.up=true;GameTestProbe::update(g,u,16);assert(GameTestProbe::s(g).selected==0);assert(count(g.takeDeathAudioEvents(),0x15u)==1);std::cout<<"settings nav audio r312 PASS\n";}

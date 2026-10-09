#include <cassert>
#include <iostream>
#include "game/game.hpp"
#include "game/legacy_controls_trace.hpp"
struct GameTestProbe{static void enter(Game&g){g.enterControlsRemap();g.controlsRemap_.fadeCounter=kLegacyControlsTrace.fadeMax;g.controlsRemap_.readyForInput=true;}};
static int count(const std::vector<DeathAudioEvent>&v,unsigned id){int n=0;for(const auto&e:v)if(e.kind==DeathAudioEventKind::SimplePlay&&e.logicalId==id)++n;return n;}
int main(){Game g;GameTestProbe::enter(g);g.takeDeathAudioEvents();InputState i{};i.legacyPressedCode=0x50;g.update(i,16);assert(count(g.takeDeathAudioEvents(),0x15u)==0);i={};i.legacyPressedCode=0x44;g.update(i,16);assert(count(g.takeDeathAudioEvents(),0x15u)==1);i={};i.legacyPressedCode=0x44;g.update(i,16);assert(count(g.takeDeathAudioEvents(),0x15u)==0);std::cout<<"controls assign audio r314 PASS\
";}

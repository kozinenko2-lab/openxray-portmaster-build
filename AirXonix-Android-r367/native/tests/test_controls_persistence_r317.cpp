#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "game/game.hpp"
#include "game/legacy_controls_trace.hpp"
struct GameTestProbe{static void enter(Game&g){g.enterControlsRemap();g.controlsRemap_.fadeCounter=kLegacyControlsTrace.fadeMax;g.controlsRemap_.readyForInput=true;}static void commit(Game&g){g.controlsRemap_.temporary={{0x44,0x46,0x47,0x48}};g.controlsRemap_.assigned=4;g.controlsRemap_.awaitingConfirm=true;}};
int main(){Game g;GameTestProbe::enter(g);g.takeDeathAudioEvents();GameTestProbe::commit(g);InputState e{};e.legacyPressedCode=0x0d;g.update(e,16);while(g.phase()==GamePhase::Controls){InputState n{};g.update(n,100);}assert(g.settings().bindings[0]==0x44);assert(g.settings().bindings[3]==0x48);std::cout<<"controls persistence r317 PASS\
";}

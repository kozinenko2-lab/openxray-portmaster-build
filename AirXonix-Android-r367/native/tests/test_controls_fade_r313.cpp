#include <cassert>
#include <cmath>
#include <iostream>
#include "game/game.hpp"
#include "game/legacy_controls_trace.hpp"
struct GameTestProbe{static void enter(Game&g){g.enterControlsRemap();}};
int main(){
 static_assert(kLegacyControlsTrace.fadeMax==0x7c0);
 static_assert(kLegacyControlsTrace.fadeInRate==3);
 static_assert(kLegacyControlsTrace.fadeOutRate==-4);
 Game g; GameTestProbe::enter(g); g.takeDeathAudioEvents();
 InputState d{}; d.legacyPressedCode=0x44; g.update(d,100);
 assert(g.controlsRemap().assigned==0);
 while(!g.controlsRemap().readyForInput){InputState n{};g.update(n,100);}
 assert(g.controlsRemap().fadeCounter==0x7c0);
 assert(std::fabs(kLegacyControlsTrace.modelLightScale(0x7c0)-(248.f/255.f))<1e-6f);
 d={};d.legacyPressedCode=0x44;g.update(d,16);assert(g.controlsRemap().assigned==1);
 InputState esc{};esc.legacyPressedCode=0x1b;g.update(esc,16);assert(g.phase()==GamePhase::Controls);assert(g.controlsRemap().exitPending);
 while(g.phase()==GamePhase::Controls){InputState n{};g.update(n,100);}
 assert(g.phase()==GamePhase::Settings);
 std::cout<<"controls fade r313 PASS\
";
}

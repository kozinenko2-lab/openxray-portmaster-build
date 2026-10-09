#include "game/game.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
struct GameTestProbe { static void enter(Game& g){g.enterModeSelect();} static void setMode(Game& g,int v){g.legacySelectedMode_=v;} };
static int countSimple(const std::vector<DeathAudioEvent>& ev,std::size_t id){int n=0;for(auto&e:ev)n+=e.kind==DeathAudioEventKind::SimplePlay&&e.logicalId==id;return n;}
int main(){
  Game g;g.takeDeathAudioEvents();GameTestProbe::setMode(g,3);GameTestProbe::enter(g);
  assert(g.modeSelect().selected==3 && !g.modeSelect().readyForInput);
  InputState up{};up.up=true;g.update(up,100);assert(g.modeSelect().selected==3);g.takeDeathAudioEvents();
  InputState none{};for(int i=0;i<10 && !g.modeSelect().readyForInput;++i)g.update(none,100);
  assert(g.modeSelect().readyForInput && g.modeSelect().fadeCounter==0x7c0);
  g.update(none,1); // fresh-key release
  g.update(up,1);assert(g.modeSelect().selected==2);auto ev=g.takeDeathAudioEvents();assert(countSimple(ev,0x15)==1);
  g.update(none,1);
  InputState back{};back.back=true;g.update(back,1);ev=g.takeDeathAudioEvents();assert(countSimple(ev,0x16)==1);assert(g.phase()==GamePhase::ModeSelect);
  for(int i=0;i<10 && g.phase()==GamePhase::ModeSelect;++i)g.update(none,100);
  assert(g.phase()==GamePhase::MainMenu);
  std::cout<<"mode select lifecycle r305 PASS\n";
}

#include "game/game.hpp"
#include <cassert>
#include <iostream>
static std::size_t cue(const std::vector<DeathAudioEvent>& ev){
  for(const auto& e:ev) if(e.kind==DeathAudioEventKind::SpatialPlay && (e.logicalId==41 || e.logicalId==42)) return e.logicalId;
  return 0;
}
int main(){
  Game g; g.setSpeechEnabled(true); g.takeDeathAudioEvents();
  for(int i=0;i<4 && g.levelIntroActive();++i)g.update({},1000);
  auto ev=g.takeDeathAudioEvents();
  assert(cue(ev)==42);
  // Finish entry and force a current-level restart through the public keyboard path.
  for(int i=0;i<20 && g.levelEntryActive();++i)g.update({},100);
  if(g.levelEntryActive())g.update({},1);
  InputState r{}; r.legacyPressedCode=0x08; // Backspace
  g.update(r,1);
  g.takeDeathAudioEvents();
  for(int i=0;i<4 && g.levelIntroActive();++i)g.update({},1000);
  ev=g.takeDeathAudioEvents();
  assert(cue(ev)==41);
  std::cout<<"level entry speech r296 PASS\n";
}

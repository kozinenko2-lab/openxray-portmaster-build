#include <cassert>
#include "game/game.hpp"

struct GameTestProbe {
  static void spawn(Game& g,int slot,float x,float y,float z){g.spawnAuxiliaryEffect(slot,x,y,z);}
  static void step(Game& g,int dt){g.updateAuxiliaryEffects(dt);}
  static void speech(Game& g,bool v){g.speechEnabled_=v;}
};

int main(){
  Game g;
  GameTestProbe::spawn(g,0,.5f,.01f,.5f);
  assert(g.auxiliaryEffects()[0].y==.02f && g.auxiliaryEffects()[0].trigger);
  GameTestProbe::speech(g,false);
  GameTestProbe::step(g,400); // +.02 => .04, but trigger stays armed when speech is off
  assert(g.auxiliaryEffects()[0].trigger);
  assert(g.takeDeathAudioEvents().empty());
  GameTestProbe::speech(g,true);
  GameTestProbe::step(g,1);
  auto ev=g.takeDeathAudioEvents();
  assert(ev.size()==1);
  assert(ev[0].kind==DeathAudioEventKind::SpatialPlay && ev[0].logicalId==0x21u);
  assert(ev[0].scalar==1.2f);
  assert(!g.auxiliaryEffects()[0].trigger);

  GameTestProbe::spawn(g,5,.4f,.008f,.6f);
  const float y0=g.auxiliaryEffects()[5].y;
  GameTestProbe::step(g,1000);
  assert(g.auxiliaryEffects()[5].y==y0+0.025f);
  assert(g.takeDeathAudioEvents().empty()); // timeout slot never speaks
}

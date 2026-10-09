#include "game/legacy_controls_trace.hpp"
#include "render/legacy_hud.hpp"
#include <cassert>
#include <iostream>
int main(){
  assert(LegacyControlsTrace::textPaletteLevel(0)==0);
  assert(LegacyControlsTrace::textPaletteLevel(0x7c0)==31);
  assert(LegacyControlsTrace::fadeTextByte(255,0x3e0)==123);
  LegacyHudState s{}; s.screen=LegacyHudScreen::Controls; s.controlsFadeCounter=0;
  auto a=LegacyHud::compose(s); assert(!a.empty()); for(const auto& q:a) assert(q.r==0.f&&q.g==0.f&&q.b==0.f);
  s.controlsFadeCounter=0x7c0; auto b=LegacyHud::compose(s); bool bright=false; for(const auto& q:b) if(q.r>0.f||q.g>0.f||q.b>0.f){bright=true;break;} assert(bright);
  std::cout<<"controls text palette r332 PASS\n";
}

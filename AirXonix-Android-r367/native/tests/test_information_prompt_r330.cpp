#include "game/legacy_information_transition.hpp"
#include "render/legacy_hud.hpp"
#include <cassert>
#include <iostream>
int main(){
  using namespace LegacyInformationTransition;
  assert(advancePromptPhase(0,100)==200);
  assert(promptRedByte(0)==190);
  LegacyHudState s{}; s.screen=LegacyHudScreen::Information; s.informationPage=0; s.informationFadeCounter=0x7c0; s.informationPromptPhase=0;
  auto q=LegacyHud::compose(s); bool found=false;
  for(const auto& g:q) if(g.y==14.f*32.f){found=true; assert(g.r==190.f/255.f); assert(g.g==0.f&&g.b==0.f);}
  assert(found); std::cout<<"information prompt r330 PASS\n";
}

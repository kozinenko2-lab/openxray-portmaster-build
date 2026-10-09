#include "game/legacy_information_transition.hpp"
#include "render/legacy_hud.hpp"
#include <cassert>
#include <iostream>
int main(){
  using namespace LegacyInformationTransition;
  assert(textPaletteLevel(0)==0); assert(textPaletteLevel(0x7c0)==31);
  assert(fadeTextByte(255,0x3e0)==123);
  LegacyHudState s{}; s.screen=LegacyHudScreen::Information; s.informationPage=0; s.informationFadeCounter=0;
  auto a=LegacyHud::compose(s); assert(!a.empty()); assert(a[0].r==0.f&&a[0].g==0.f&&a[0].b==0.f);
  s.informationFadeCounter=0x7c0; auto b=LegacyHud::compose(s); assert(b[0].g==1.f);
  std::cout<<"information text fade r329 PASS\n";
}

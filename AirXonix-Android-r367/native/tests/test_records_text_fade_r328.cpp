#include "game/legacy_records_transition.hpp"
#include "render/legacy_hud.hpp"
#include <cassert>
#include <iostream>
int main(){
    using namespace LegacyRecordsTransition;
    assert(textPaletteLevel(0)==0); assert(textPaletteLevel(0x7c0)==31);
    assert(fadeTextByte(255,0x3e0)==123); // level 15
    LegacyHudState s{}; s.screen=LegacyHudScreen::Records; s.recordsFadeCounter=0;
    auto dark=LegacyHud::compose(s); assert(!dark.empty()); assert(dark[0].r==0.f && dark[0].g==0.f && dark[0].b==0.f);
    s.recordsFadeCounter=0x7c0; auto full=LegacyHud::compose(s); assert(full[0].r==1.f);
    std::cout<<"records text fade r328 PASS\n";
}

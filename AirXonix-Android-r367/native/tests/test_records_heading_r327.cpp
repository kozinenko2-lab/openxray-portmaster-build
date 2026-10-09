#include "game/legacy_records_transition.hpp"
#include "render/legacy_hud.hpp"
#include <cassert>
#include <iostream>
int main(){
    using namespace LegacyRecordsTransition;
    assert(advanceHeadingPhase(0,100)==400);
    assert(advanceHeadingPhase(2000,20)==32);
    assert(headingWaveRgb(0,0)==0x000200u);
    LegacyHudState s{}; s.screen=LegacyHudScreen::Records; s.recordsHeadingPhase=0;
    auto q=LegacyHud::compose(s);
    // first heading glyph remains red; second (column 15) gets wave colour.
    assert(q[0].r==1.f && q[0].g==0.f);
    const auto c=headingWaveRgb(0,0); const auto& g=q[1];
    assert(g.r==float((c>>16)&255)/255.f && g.g==float((c>>8)&255)/255.f);
    std::cout<<"records heading r327 PASS\n";
}

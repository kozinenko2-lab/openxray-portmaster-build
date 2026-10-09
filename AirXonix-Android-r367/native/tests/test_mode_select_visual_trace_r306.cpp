#include "render/legacy_theme.hpp"
#include "game/game.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
    const auto& t=kLegacyModeSelectPresentationTrace;
    assert(t.function==0x00411870u);
    assert(t.slot0Prepared==0x025B5AE8u);
    assert(t.airbornePrepared==0x02583748u);
    assert(t.crawlerPrepared==0x0257F5B8u);
    assert(LegacyModeSelectPresentationTrace::lightByte(0)==0);
    assert(LegacyModeSelectPresentationTrace::lightByte(0x7c0)==248);
    assert(std::fabs(LegacyModeSelectPresentationTrace::airborneZ(0)-(-.003f))<1e-7f);
    assert(std::fabs(LegacyModeSelectPresentationTrace::airborneZ(4)-(-.010f))<2e-7f);
    assert(std::fabs(LegacyModeSelectPresentationTrace::crawlerScale(0)-0.15000000596046448f)<1e-7f);
    assert(LegacyModeSelectPresentationTrace::crawlerScale(314)>0.199f);
    assert(LegacyModeSelectPresentationTrace::crawlerScale(942)<0.101f);
    Game g; g.showMainMenuOnBoot();
    // enter mode selector through public main-menu path: release then action.
    InputState none{}; g.update(none,1);
    InputState action{}; action.action=true; g.update(action,1);
    // MainMenu dispatch is delayed; give the menu enough time to enter selector.
    for(int i=0;i<5000 && g.phase()!=GamePhase::ModeSelect;++i) g.update(none,1);
    assert(g.phase()==GamePhase::ModeSelect);
    const int a0=g.modeSelect().anglePhase; const float p0=g.modeSelect().backgroundPhase;
    g.update(none,20);
    assert(g.modeSelect().anglePhase==a0+40);
    float expect=p0+20.f*0.0005000000237487257f; if(expect>=1.f)expect-=1.f;
    assert(std::fabs(g.modeSelect().backgroundPhase-expect)<1e-6f);
    std::cout<<"mode select visual trace r306 PASS\n";
}

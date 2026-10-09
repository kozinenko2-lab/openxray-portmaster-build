#include "render/legacy_hud.hpp"
#include "render/legacy_hud_trace.hpp"
#include <cassert>
#include <iostream>

int main(){
    LegacyHudState s; s.lives=3; s.timeRemaining=30000; s.levelNumber=4; s.score=12345; s.capturePercent=76;
    const auto hud=LegacyHud::compose(s);
    assert(!hud.empty());
    bool hasHeart=false,hasPercent=false;
    for(const auto& q:hud){
        const auto& heart=kLegacyGameplayHudStaticQuads[0];
        const auto& percent=kLegacyGameplayHudStaticQuads[4];
        hasHeart|=(q.atlas==LegacyHudAtlas::Atlas3&&q.x==heart.x640&&q.y==heart.y480&&q.u0==heart.u0&&q.v0==heart.v0);
        hasPercent|=(q.atlas==LegacyHudAtlas::Atlas3&&q.x==percent.x640&&q.y==percent.y480&&q.u0==percent.u0&&q.v0==percent.v0);
    }
    assert(hasHeart&&hasPercent);

    s.screen=LegacyHudScreen::InterLevel; s.levelNumber=5;
    const auto inter=LegacyHud::compose(s);
    // r49 and later renderer integration: InterLevel keeps the normal atlas-3
    // gameplay HUD alive. What was disproved is the invented texture-7
    // LEV2/CNT3 overlay, not the five-field gameplay HUD itself.
    assert(!inter.empty());
    for(const auto& q:inter) assert(q.atlas!=LegacyHudAtlas::Atlas7 && q.atlas!=LegacyHudAtlas::Atlas4);

    s.screen=LegacyHudScreen::Complete; const auto complete=LegacyHud::compose(s);
    assert(complete.empty()); // r141: no standalone Complete HUD; COMP is cinematic slot 0 inside finale.
    s.screen=LegacyHudScreen::GameOver; const auto over=LegacyHud::compose(s);
    assert(over.empty()); // r132: GOVE is cinematic slot 3, not a HUD sprite.
    s.screen=LegacyHudScreen::Abort; const auto abort=LegacyHud::compose(s);
    assert(abort.empty()); // r133: ABOR is cinematic slot 4.
    s.screen=LegacyHudScreen::MainMenu; s.menuSelected=0; s.menuBrightness={{1.0f,0.6f,0.6f,0.6f,0.6f}}; s.menuScale={{1.15f,1.0f,1.0f,1.0f,1.0f}}; const auto menu=LegacyHud::compose(s);
    assert(menu.size()==5);
    assert(menu[0].atlas==LegacyHudAtlas::MenuM1 && menu[0].sy==0);
    assert(menu[4].atlas==LegacyHudAtlas::MenuM1 && menu[4].sy==192);
    assert(menu[0].brightness==1.0f);
    assert(menu[1].brightness==0.6f);
    assert(menu[0].w>menu[1].w); // selected target scale 1.15 versus 1.0

    // r342 DIRECT EXE 0x4119D6..0x411A68: five fixed-width 11-byte
    // SOUNDINF names are rendered at column 8 on rows 7..11.
    LegacyHudState ms; ms.screen=LegacyHudScreen::ModeSelect; ms.modeSelected=0; ms.modeFadeCounter=0x7c0;
    ms.modeNames={{"EASY","CLASSIC","MODERN","HARD","EXTREME"}};
    ms.modeLevelCounts={{7,15,20,20,20}};
    const auto modeHud=LegacyHud::compose(ms);
    int firstRowNameGlyphs=0;
    for(const auto& q:modeHud) if(q.atlas==LegacyHudAtlas::Font5 && q.y==7.f*32.f && q.x>=8.f*16.f && q.x<19.f*16.f) ++firstRowNameGlyphs;
    assert(firstRowNameGlyphs==11);
    std::cout<<"legacy_hud sprites="<<hud.size()<<" inter="<<inter.size()<<"\n";
}

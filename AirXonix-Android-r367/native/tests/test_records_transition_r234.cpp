#include "game/game.hpp"
#include "game/legacy_records_transition.hpp"
#include "render/legacy_theme.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

struct GameTestProbe {
    static void enter(Game& g){g.enterRecords();}
    static const RecordsState& state(const Game& g){return g.records_;}
};

int main(){
    using namespace LegacyRecordsTransition;
    static_assert(FadeMax==0x7C0);
    static_assert(FadeInRate==3);
    static_assert(FadeOutRate==-4);
    static_assert(NavigateSfx==0x15u);
    static_assert(ExitSfx==0x16u);
    static_assert(ModeCount==5u);

    assert(kLegacyRecordsBackgroundTrace.texture1Select==0x0040FA74u);
    assert(kLegacyRecordsBackgroundTrace.quadCall==0x0040FAE7u);
    assert(std::fabs(kLegacyRecordsBackgroundTrace.phaseStep-0.0003000000142492354f)<1e-10f);
    assert(backdropByte(FadeMax)==0x7c);
    assert(modelLightByte(FadeMax)==0xf8);
    assert(previousMode(0)==4u && nextMode(4)==0u);

    Game g;
    GameTestProbe::enter(g);
    InputState in{};
    for(int i=0;i<6;++i)g.update(in,100);
    assert(GameTestProbe::state(g).fadeCounter==1800);
    assert(!GameTestProbe::state(g).readyForInput);
    g.update(in,100);
    assert(GameTestProbe::state(g).fadeCounter==FadeMax);
    assert(GameTestProbe::state(g).readyForInput);
    assert(!GameTestProbe::state(g).inputLatched);

    // DIRECT EXE: previous wraps mode 0 -> last and plays SFX 0x15.
    in.left=true;
    g.update(in,16);
    assert(GameTestProbe::state(g).mode==4u);
    auto ev=g.takeDeathAudioEvents();
    assert(!ev.empty() && ev.back().kind==DeathAudioEventKind::SimplePlay && ev.back().logicalId==NavigateSfx);
    in={}; g.update(in,16);

    // Exit starts -4 fade and does not return to M1 immediately.
    in.action=true;
    g.update(in,16);
    assert(g.phase()==GamePhase::Records);
    assert(GameTestProbe::state(g).fadeRate==FadeOutRate);
    ev=g.takeDeathAudioEvents();
    assert(!ev.empty() && ev.back().logicalId==ExitSfx);
    in={};
    g.update(in,497); // 1984 - 4*497 = -4, so return is now legal
    assert(g.phase()==GamePhase::MainMenu);
    std::cout << "records transition r234 PASS\n";
}

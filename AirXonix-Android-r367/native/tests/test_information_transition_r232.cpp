#include "game/game.hpp"
#include "game/legacy_information_transition.hpp"
#include <cassert>
#include <iostream>

struct GameTestProbe {
    static void enter(Game& g){g.enterInformation();}
    static const InformationState& state(const Game& g){return g.information_;}
};

int main(){
    using namespace LegacyInformationTransition;
    static_assert(FadeMax==0x7C0);
    static_assert(FadeInRate==3);
    static_assert(FadeOutRate==-4);
    static_assert(AdvanceSfx==0x16u);

    Game g;
    GameTestProbe::enter(g);
    InputState in{};
    for(int i=0;i<6;++i)g.update(in,100); // 1800, still not armed
    assert(GameTestProbe::state(g).fadeCounter==1800);
    assert(!GameTestProbe::state(g).readyForInput);
    g.update(in,100); // 2100 -> clamp 0x7C0 and arm fresh-input gate
    assert(GameTestProbe::state(g).fadeCounter==FadeMax);
    assert(GameTestProbe::state(g).readyForInput);
    assert(!GameTestProbe::state(g).inputLatched); // released input clears latch

    in.action=true;
    g.update(in,16);
    assert(g.phase()==GamePhase::Information);
    assert(GameTestProbe::state(g).page==0);
    assert(GameTestProbe::state(g).fadeRate==FadeOutRate);
    auto ev=g.takeDeathAudioEvents();
    // entry music events plus the exact page-advance SFX are queued in this test.
    assert(!ev.empty() && ev.back().kind==DeathAudioEventKind::SimplePlay && ev.back().logicalId==AdvanceSfx);

    in={};
    g.update(in,497);
    assert(GameTestProbe::state(g).page==1);
    assert(GameTestProbe::state(g).fadeCounter==0);
    assert(GameTestProbe::state(g).fadeRate==FadeInRate);
    assert(!GameTestProbe::state(g).readyForInput);
    assert(GameTestProbe::state(g).inputLatched);
    std::cout << "information transition r232 PASS\n";
}

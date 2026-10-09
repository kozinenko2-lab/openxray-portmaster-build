#include <cassert>
#include <cmath>
#include "game/legacy_restore_trace.hpp"
#include "game/game.hpp"
struct GameTestProbe { static void gameplay(Game& g){g.phase_=GamePhase::Gameplay; g.levelIntroScene_.elapsedMs=g.levelIntroScene_.durationMs; g.levelEntryScene_.running=false;} };
int main(){
    static_assert(LegacySessionRestore.routine==0x00419250u);
    assert(LegacySessionRestore.onlyCaller==0x00424E7Eu);
    assert(LegacySessionRestore.levelAddress==0x0257DA18u);
    assert(LegacySessionRestore.scoreAddress==0x0257DA20u);
    assert(LegacySessionRestore.livesAddress==0x0257DA10u);
    assert(LegacyAbortConfirm.panelPrepared==0x0257F5DCu);
    assert(std::fabs(LegacyAbortConfirm.enterRatePerMs-0.0008f)<1e-7f);
    Game g; GameTestProbe::gameplay(g);
    InputState in{}; in.select=true; g.update(in,16); assert(g.phase()==GamePhase::Abort);
    in={}; for(int i=0;i<30 && g.abortConfirm().stage==AbortConfirmState::Stage::Entering;++i)g.update(in,16);
    assert(g.abortConfirm().stage==AbortConfirmState::Stage::Holding);
    g.update(in,16); // arm after release
    // B cancels the Select/Esc abort modal.
    in.back=true; g.update(in,16); assert(!g.abortConfirm().confirmed);
    in={}; for(int i=0;i<40 && g.phase()==GamePhase::Abort;++i)g.update(in,16);
    assert(g.phase()==GamePhase::Gameplay);

    // Re-open with Select and confirm with A.
    in.select=true; g.update(in,16); assert(g.phase()==GamePhase::Abort);
    in={}; for(int i=0;i<30 && g.abortConfirm().stage==AbortConfirmState::Stage::Entering;++i)g.update(in,16);
    g.update(in,16);
    in.action=true; g.update(in,16); assert(g.abortConfirm().confirmed);
    in={}; for(int i=0;i<40 && g.phase()==GamePhase::Abort;++i)g.update(in,16);
    assert(g.phase()==GamePhase::MainMenu);
}

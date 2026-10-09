#include <cassert>
#include <cmath>
#include "game/game.hpp"
#include "game/legacy_pause_trace.hpp"
#include "render/legacy_model_factory.hpp"
struct GameTestProbe { static void gameplay(Game& g){g.phase_=GamePhase::Gameplay; g.levelIntroScene_.elapsedMs=g.levelIntroScene_.durationMs; g.levelEntryScene_.running=false;} };
int main(){
    static_assert(kLegacyPauseTrace.routine==0x0041DE30u);
    static_assert(kLegacyPauseTrace.panelPrepared==0x0257F5D4u);
    const auto mesh=LegacyModelFactory::buildCinematicSlot(2);
    assert(!mesh.vertices.empty() && !mesh.faces.empty());

    Game g; GameTestProbe::gameplay(g);
    InputState in{}; in.pause=true; g.update(in,16);
    assert(g.paused());
    assert(g.pauseScene().stage==PauseSceneState::Stage::Entering);
    assert(std::fabs(g.pauseScene().panelY-kLegacyPauseTrace.enterStartY)<1e-6f);
    auto ev=g.takeDeathAudioEvents();
    bool click=false; for(const auto& e:ev) if(e.kind==DeathAudioEventKind::SimplePlay && e.logicalId==0x16u) click=true;
    assert(click);

    in={};
    for(int i=0;i<40 && g.pauseScene().stage==PauseSceneState::Stage::Entering;++i)g.update(in,16);
    assert(g.pauseScene().stage==PauseSceneState::Stage::Holding);
    assert(std::fabs(g.pauseScene().panelY-kLegacyPauseTrace.holdY)<1e-6f);
    g.update(in,16); // arm after release
    in.pause=true; g.update(in,16);
    assert(g.pauseScene().stage==PauseSceneState::Stage::Leaving);
    in={};
    for(int i=0;i<50 && g.paused();++i)g.update(in,16);
    assert(!g.paused());
}

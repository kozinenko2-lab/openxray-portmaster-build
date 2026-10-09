#include <cassert>
#include "game/game.hpp"
#include "render/legacy_hud.hpp"

struct GameTestProbe {
    static void finale(Game& g){ g.levelIntroScene_.elapsedMs=g.levelIntroScene_.durationMs; g.levelEntryScene_.running=false; g.beginFinalSequence(); }
};

int main(){
    Game g;
    GameTestProbe::finale(g);
    assert(g.phase()==GamePhase::FinalSequence);
    // One large synthetic step is sufficient because the native state uses
    // accumulated elapsed time for the terminal return condition.
    g.update(InputState{},92550);
    assert(g.phase()==GamePhase::Records);

    LegacyHudState s{}; s.screen=LegacyHudScreen::Complete;
    assert(LegacyHud::compose(s).empty());
    return 0;
}

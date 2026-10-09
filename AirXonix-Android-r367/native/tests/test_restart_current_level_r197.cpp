#include "game/game.hpp"
#include "game/legacy_restore_trace.hpp"
#include <cassert>
#include <cstddef>

struct GameTestProbe {
    static void enterLevel(Game& g,std::size_t level,int score,int lives){
        g.score_=score; g.lives_=lives;
        g.loadLevel(g.mode_,level); // r267: this captures DAEC/DAF0 level-entry checkpoint.
    }
    static void mutateAttempt(Game& g,int score,int lives,int timer){
        g.score_=score; g.lives_=lives; g.timer_=timer; g.phase_=GamePhase::Gameplay; g.levelIntroScene_.elapsedMs=g.levelIntroScene_.durationMs; g.levelEntryScene_.running=false;
    }
};

int main(){
    Game g;
    assert(g.restartCredits()==kLegacyRestartCurrentLevelTrace.initialCredits);

    // r267: the checkpoint is the state at level entry, not the state at the
    // instant Backspace is pressed.
    GameTestProbe::enterLevel(g,1,43210,2);
    GameTestProbe::mutateAttempt(g,50000,1,7777);
    InputState restart{};
    restart.legacyPressedCode=kLegacyRestartCurrentLevelTrace.keyboardKey;
    g.update(restart,16);

    assert(g.phase()==GamePhase::Gameplay);
    assert(g.levelIndex()==1);
    assert(g.score()==43210);
    assert(g.lives()==2);
    assert(g.timeRemaining()==g.settings().testInitialTimeSeconds*1024);
    assert(g.restartCredits()==4);

    // The restored state becomes the new entry checkpoint for the rebuilt level.
    GameTestProbe::mutateAttempt(g,99999,1,1234);
    g.update(restart,16);
    assert(g.score()==43210);
    assert(g.lives()==2);
    assert(g.restartCredits()==3);
    return 0;
}

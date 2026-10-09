#include "game/game.hpp"
#include <cassert>
#include <iostream>
struct GameTestProbe {
    static void arm(Game& g){
        g.phase_=GamePhase::Gameplay; g.paused_=false;
        g.levelIntroScene_.elapsedMs=g.levelIntroScene_.durationMs;
        g.levelEntryScene_.running=false;
        g.timer_=2000; g.hudDisplayTimer_=1995;
        g.score_=0; g.hudDisplayScore_=0;
    }
};
int main(){
    Game g; GameTestProbe::arm(g);
    g.update(InputState{},10);
    // Gameplay first decrements actual timer to 1990; HUD then chases the new
    // value and therefore snaps down to 1990. Pre-r298 ordering produced 2000.
    assert(g.timeRemaining()==1990);
    assert(g.hudDisplayTimer()==1990);
    std::cout<<"hud post-state r298 PASS\n";
}

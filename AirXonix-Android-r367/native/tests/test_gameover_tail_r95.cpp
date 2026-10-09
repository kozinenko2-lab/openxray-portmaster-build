#include "core/legacy_highscore.hpp"
#define private public
#include "game/game.hpp"
#undef private
#include <cassert>
#include <iostream>

int main(){
    Game g;
    g.lives_=1;
    g.handleDeath();
    assert(g.phase_==GamePhase::Dying);
    g.updateDeathSequence(4000);
    assert(g.phase_==GamePhase::Dying);
    g.updateDeathSequence(1);
    assert(g.phase_==GamePhase::GameOver);
    assert(g.gameOverScene_.timeoutMs==60000);
    // The crossing-frame overshoot is preserved by the original stack-local countdown.
    assert(g.gameOverScene_.elapsedMs==1);
    assert(g.deathTriColorInitialized_==g.gameOverScene_.visual.triColorBurstStarted);

    // Empty queue / no input: original stays in the zero-lives tail.
    InputState idle{};
    g.updateGameOverTail(idle,59999);
    assert(g.phase_==GamePhase::GameOver);
    assert(g.gameOverScene_.elapsedMs==60000);

    // 0x41C6FE uses signed < -60000, so equality is still alive.
    g.updateGameOverTail(idle,1);
    assert(g.phase_==GamePhase::Records);

    // A queued input event also exits, but only after release-arming.
    Game h;
    h.lives_=1;
    h.handleDeath();
    h.updateDeathSequence(4000);
    assert(h.phase_==GamePhase::Dying);
    h.updateDeathSequence(1);
    assert(h.phase_==GamePhase::GameOver);
    InputState held{}; held.action=true;
    h.updateGameOverTail(held,16);
    assert(h.phase_==GamePhase::GameOver);
    h.updateGameOverTail(idle,16); // arm after release
    assert(h.phase_==GamePhase::GameOver);
    h.updateGameOverTail(held,16);
    assert(h.phase_==GamePhase::Records);

    std::cout << "gameover tail r95 ok\n";
}

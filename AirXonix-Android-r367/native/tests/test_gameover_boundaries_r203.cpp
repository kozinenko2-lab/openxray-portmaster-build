#include "core/legacy_highscore.hpp"
#define private public
#include "game/game.hpp"
#undef private
#include <cstdlib>
#include <iostream>

namespace {
void check(bool ok, const char* msg) {
    if (!ok) {
        std::cerr << "FAIL: " << msg << "\n";
        std::exit(1);
    }
}
}

int main() {
    InputState idle{};

    // DIRECT EXE 0x41C625: death presentation exits only after signed
    // remaining crosses below zero. Equality at zero is still Dying.
    Game normal;
    normal.lives_ = 2;
    normal.handleDeath();
    normal.updateDeathSequence(4000);
    check(normal.phase_ == GamePhase::Dying, "remaining==0 must still be Dying");
    check(normal.deathScene_.remainingMs() == 0, "death remaining must be exactly zero");
    normal.updateDeathSequence(1);
    check(normal.phase_ == GamePhase::Gameplay, "remaining<0 must respawn a surviving player");

    // Last life uses the same strict edge and preserves the one-millisecond
    // crossing-frame overshoot into the Game Over tail.
    Game terminal;
    terminal.lives_ = 1;
    terminal.handleDeath();
    terminal.updateDeathSequence(4000);
    check(terminal.phase_ == GamePhase::Dying, "last-life equality must still be Dying");
    terminal.updateDeathSequence(1);
    check(terminal.phase_ == GamePhase::GameOver, "last-life negative crossing must enter GameOver");
    check(terminal.gameOverScene_.elapsedMs == 1, "crossing overshoot must be preserved");

    // DIRECT EXE 0x41C6FE: signed countdown exits only below -60000.
    terminal.updateGameOverTail(idle, 59999);
    check(terminal.phase_ == GamePhase::GameOver, "GameOver at exactly 60000ms must remain active");
    check(terminal.gameOverScene_.elapsedMs == 60000, "GameOver elapsed equality mismatch");

    // Timeout check is before the common live-world update in that frame.
    const float phaseBeforeExit = terminal.legacyBackgroundVPhase();
    terminal.updateGameOverTail(idle, 1);
    check(terminal.phase_ == GamePhase::Records, "GameOver must enter post-game Records only after 60000ms");
    check(terminal.legacyBackgroundVPhase() == phaseBeforeExit,
          "timeout-crossing frame must not execute live-world background update");

    std::cout << "gameover strict boundaries r203 ok\n";
    return 0;
}

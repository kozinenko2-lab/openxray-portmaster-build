#include "game/game.hpp"
#include "game/legacy_level_init_trace.hpp"
#include "game/level.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

struct GameTestProbe {
    static int denominator(const Game& g){ return g.captureDenominator_; }
    static int initialOccupied(const Game& g){ return g.initialOccupied_; }
    static float scoreMultiplier(const Game& g){ return g.scoreMultiplier_; }
    static void load(Game& g,std::size_t m,std::size_t l){ g.loadLevel(m,l); }
};

int main(){
    const auto& t=kLegacyLevelInit;
    assert(t.routine==0x00418DD0u && t.levelRecordBytes==28u);
    assert(t.fieldCells==4096);
    assert(std::fabs(t.crawlerWeight-.7f)<1e-6f);
    assert(std::fabs(t.airborneBWeight-1.5f)<1e-6f);
    assert(std::fabs(t.specialWeight-3.f)<1e-6f);
    assert(std::fabs(t.baseScoreTerm-10.f)<1e-6f);
    assert(std::fabs(t.finalScoreScale-.05f)<1e-6f);
    assert(t.enemyACapturePenalty==100 && t.enemyBCapturePenalty==120 && t.eraserCapturePenalty==100);
    assert(t.timerShift==10 && t.initialPickupDelayMs==5000 && t.pickupCount==6);

    Game g;
    GameTestProbe::load(g,0,0);
    const auto& r=g.database().level(0,0);
    const int expectedDen=4096-GameTestProbe::initialOccupied(g)
        -100*int(r.enemyTypeACount)-120*int(r.enemyTypeBCount)-(r.specialEraser?100:0);
    assert(GameTestProbe::denominator(g)==(expectedDen<1?1:expectedDen));
    assert(g.timeRemaining()==(60<<10));
    assert(std::fabs(GameTestProbe::scoreMultiplier(g)-legacyScoreMultiplier(r,g.timeScale()))<1e-6f);
    for(const auto& p:g.pickups().items()) assert(p.timerMs==5000);
    assert(std::fabs(g.brightnessScale()-1.0f)<1e-7f);

    std::cout << "level init r110 PASS\n";
}

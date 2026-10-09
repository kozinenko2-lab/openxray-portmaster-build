#include "game/legacy_progress_trace.hpp"
#include "game/pickups.hpp"
#include "game/field.hpp"
#include "core/legacy_random.hpp"
#include <cassert>
#include <iostream>

int main(){
    const auto& t=kLegacyProgressUpdate;
    assert(t.routine==0x004194D0u);
    assert(t.fieldBase==0x025849DCu && t.fieldCells==4096 && t.occupancyMask==0x3f);
    assert(t.capturePercentAddress==0x0257DA1Cu && t.scoreAddress==0x0257DA20u);
    assert(t.bonusAccumulatorAddress==0x0257DA24u);
    assert(t.bonusCellsPerLife==400 && t.extraLifePickupIndex==2 && t.forcedDelayMs==1000);
    assert(t.extraLifePickupDelayAddress==0x0254A220u);

    Field field;
    LevelRecord level{};
    field.build(level);
    LegacyRandom rng;
    Pickups pickups;
    pickups.reset(field,rng,60000);
    // r110 direct EXE trace: level bootstrap overrides all six initial pickup delays to 5000 ms.
    assert(pickups.items()[2].timerMs==5000);
    pickups.forceExtraLifePickupSoon();
    assert(pickups.items()[2].timerMs==1000);
    // The original only clamps values above 1000; it must never lengthen an
    // already-short delay.
    pickups.forceExtraLifePickupSoon();
    assert(pickups.items()[2].timerMs==1000);

    std::cout << "progress update r101 PASS\n";
}

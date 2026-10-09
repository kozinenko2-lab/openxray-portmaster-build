#include "game/legacy_level_intro_order_trace.hpp"
#include <cassert>
#include <iostream>
int main(){
    using namespace LegacyLevelIntroOrder;
    assert(kTrace.outerLevelInitCall==0x424EF1u);
    assert(kTrace.levelInitRoutine==0x418DD0u);
    assert(kTrace.gameplayRoutine==0x4195D0u);
    assert(kTrace.introDispatchCall==0x419602u);
    assert(kTrace.introDispatcher==0x41CEA0u);
    assert(kTrace.firstGameplayTick==0x4196DFu);
    assert(kTrace.introAfterLevelInit && kTrace.introBeforeTimerAndInput);
    assert(kTrace.introRepeatsOnRestart && kTrace.introRepeatsOnNextLevel);
    std::cout << "level intro order r283 PASS\n";
}

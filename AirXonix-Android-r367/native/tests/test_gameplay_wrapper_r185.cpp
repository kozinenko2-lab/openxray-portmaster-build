#include "game/legacy_menu.hpp"
#include "game/legacy_restore_trace.hpp"
#include <cassert>
#include <cmath>
int main(){
    static_assert(kLegacyGameplayCampaignTrace.routine==0x00424DE0u);
    static_assert(kLegacyGameplayCampaignTrace.gameplayLoop==0x004195D0u);
    static_assert(kLegacyGameplayCampaignTrace.nextLevel==1);
    static_assert(kLegacyGameplayCampaignTrace.abortSession==-1);
    static_assert(kLegacyGameplayCampaignTrace.initialRestartCredits==5);
    static_assert(kLegacyRestartCurrentLevelTrace.keyboardKey==0x08u);
    static_assert(kLegacyRestartCurrentLevelTrace.restartFlag==0x025B5B9Cu);
    static_assert(kLegacyRestartCurrentLevelTrace.outerRestartTarget==0x00424E2Du);
    assert(std::fabs(LegacyGameplayCampaignTrace::difficultyScale(800.f)-1.f)<1e-7f);
    assert(std::fabs(LegacyGameplayCampaignTrace::difficultyScale(1200.f)-1.1f)<1e-6f);
    return 0;
}

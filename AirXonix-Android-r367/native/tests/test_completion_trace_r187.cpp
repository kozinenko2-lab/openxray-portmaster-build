#include "game/legacy_gameplay_state_trace.hpp"
#include "game/legacy_menu.hpp"
#include <cassert>
#include <cmath>
int main(){
  assert(kLegacyGameplayStateTrace.timeBonus(60000)==3000);
  assert(kLegacyGameplayStateTrace.overCaptureBonusPerPercent==1000);
  assert(kLegacyGameplayStateTrace.finalLifeBonusBase==5000);
  assert(kLegacyGameplayStateTrace.completionBonusBase==5000);
  assert(kLegacyGameplayStateTrace.interLevelDurationMs==4000);
  assert(kLegacyGameplayStateTrace.finaleInputUnlockMs==7000);
  assert(kLegacyGameplayStateTrace.finaleAutoExitMs==90000);
  assert(std::fabs(LegacyGameplayCampaignTrace::gameTimeScale(800.f)-1.f)<1e-6f);
  assert(std::fabs(LegacyGameplayCampaignTrace::gameTimeScale(1000.f)-1.05f)<1e-6f);
  return 0;
}

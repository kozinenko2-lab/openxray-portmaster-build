#include "game/legacy_gameplay_state_trace.hpp"
#include <cassert>
int main(){
  static_assert(kLegacyGameplayStateTrace.livesAddress==0x0257DA10u);
  static_assert(kLegacyGameplayStateTrace.timeRemainingMsAddress==0x0257DA14u);
  static_assert(kLegacyGameplayStateTrace.currentLevelNumberAddress==0x0257DA18u);
  static_assert(kLegacyGameplayStateTrace.capturedPercentAddress==0x0257DA1Cu);
  static_assert(kLegacyGameplayStateTrace.scoreAddress==0x0257DA20u);
  static_assert(kLegacyGameplayStateTrace.trailActiveAddress==0x0257DA74u);
  static_assert(kLegacyGameplayStateTrace.initialLives==3);
  static_assert(kLegacyGameplayStateTrace.levelCompletePercent==100);
  static_assert(kLegacyGameplayStateTrace.nextLevelReturn==1);
  static_assert(kLegacyGameplayStateTrace.abortedReturn==-1);
  static_assert(kLegacyGameplayStateTrace.finalTimePointsPerSecond==50);
  static_assert(kLegacyGameplayStateTrace.finalLifeBonusBase==5000);
  assert((kLegacyGameplayStateTrace.baseTimeSeconds << 10)==61440);
  return 0;
}

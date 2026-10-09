#include <cassert>
#include <string_view>
#include "game/legacy_effects.hpp"
#include "game/game.hpp"
int main(){
  using namespace std::literals;
  const auto& p=kLegacyParticlePool;
  assert(p.recordCount==96 && p.recordStride==24 && p.maxSpawnPerHelperCall==16);
  assert(p.spawnY==0.006f && p.freeSlotYThreshold==0.005f);
  assert(p.sfxMinimumGapMs==20 && p.cowSelector==0x0257F4E4u && p.lastSfxTick==0x0257F538u);
  assert(p.cowLogicalIds[0]==0x1Bu && p.cowFourcc[0]=="cow2"sv);
  assert(p.cowLogicalIds[1]==0x1Cu && p.cowFourcc[1]=="cow4"sv);
  assert(p.cowLogicalIds[2]==0x1Du && p.cowFourcc[2]=="cow1"sv);
  assert(p.cowLogicalIds[3]==0x1Eu && p.cowFourcc[3]=="cow3"sv);
  static_assert(static_cast<int>(DeathAudioVoiceTag::LowTimeWarning) != static_cast<int>(DeathAudioVoiceTag::None));
}

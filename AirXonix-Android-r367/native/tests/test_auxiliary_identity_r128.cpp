#include <cassert>
#include <string_view>
#include "game/legacy_effects.hpp"
int main(){
  using namespace std::literals;
  const auto& t=kLegacyAuxiliaryIdentity;
  assert(t.logicalSfx[0]==0x21u && t.fourcc[0]=="bonu"sv && t.semantic[0]=="score-bonus"sv);
  assert(t.logicalSfx[1]==0x24u && t.fourcc[1]=="time"sv);
  assert(t.logicalSfx[2]==0x20u && t.fourcc[2]=="life"sv);
  assert(t.logicalSfx[3]==0x25u && t.fourcc[3]=="slow"sv);
  assert(t.logicalSfx[4]==0x26u && t.fourcc[4]=="acce"sv);
  assert(t.semantic[5]=="timeout"sv);
  assert(t.scalar[0]==1.2f && t.scalar[1]==1.1f && t.scalar[2]==1.4f);
  assert(t.pickupDispatcher==0x004156C0u && t.pickupCaller==0x00417BFAu && t.modelBuilder==0x00422240u);
}

#include "game/legacy_session_trace.hpp"
#include <cassert>
int main(){
  assert(kLegacySessionBootstrap.initialLives==3);
  assert(kLegacySessionBootstrap.integritySentinelAddress==0x025459B4u);
  assert(kLegacySessionBootstrap.integrityShadowAddresses[0]==0x02545960u);
  return 0;
}

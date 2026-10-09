#include <cassert>
#include "game/pickups.hpp"
#include "game/field.hpp"
#include "game/player.hpp"
#include "game/entities.hpp"
#include "game/level.hpp"
#include "core/legacy_random.hpp"

int main(){
  // Lock the directly recovered immediate-collection ID mapping separately
  // from the random selector so future audio refactors cannot merge it with
  // the delayed auxiliary announcer layer.
  auto idFor=[](PickupEffect e)->std::size_t{
    const int id=static_cast<int>(e);
    return id<=3?std::size_t(0x0A+id):(id==4?std::size_t(0x31):std::size_t(0x30));
  };
  assert(idFor(PickupEffect::Score1000)==0x0A);
  assert(idFor(PickupEffect::Time15000)==0x0B);
  assert(idFor(PickupEffect::ExtraLife)==0x0C);
  assert(idFor(PickupEffect::SlowEnemies)==0x0D);
  assert(idFor(PickupEffect::PlayerFast)==0x31);
  assert(idFor(PickupEffect::PlayerSlow)==0x30);
  assert(idFor(PickupEffect::CameraZoomIn)==0x30);
  assert(idFor(PickupEffect::Blackout)==0x30);
  assert(idFor(PickupEffect::CameraShake)==0x30);
}

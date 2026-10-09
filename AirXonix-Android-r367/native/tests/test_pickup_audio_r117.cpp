#include "game/pickups.hpp"
#include "game/field.hpp"
#include "game/player.hpp"
#include "game/entities.hpp"
#include "core/legacy_random.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

int main(){
    Field field;
    LegacyRandom rng(1);
    Pickups pickups;
    Player player;
    Entities entities;

    pickups.reset(field,rng,60000);
    (void)pickups.consumeAudioEvents(); // reset has no active retained loops in a fresh session

    // First update crosses the hidden 5000-ms delay but, exactly like the EXE,
    // activation starts only on the following update.
    (void)pickups.update(5001,field,player,entities,rng,60000,3,false);
    assert(pickups.consumeAudioEvents().empty());

    // All six active pickups begin falling. The EXE starts retained spatial
    // bfly (logical ID 0x0F) and updates the same handle in the same frame.
    (void)pickups.update(1,field,player,entities,rng,60000,3,false);
    auto ev=pickups.consumeAudioEvents();
    int starts=0,updates=0;
    for(const auto& e:ev){
        if(e.kind==PickupAudioEventKind::SpatialStart){ ++starts; assert(e.logicalId==0x0Fu); }
        if(e.kind==PickupAudioEventKind::SpatialUpdate)++updates;
    }
    assert(starts==6 && updates==6);

    // A large step deliberately overshoots the surface. The original does not
    // clamp on the falling frame, so no stop/pop is emitted yet.
    (void)pickups.update(2000,field,player,entities,rng,60000,3,false);
    ev=pickups.consumeAudioEvents();
    for(const auto& e:ev)assert(e.kind!=PickupAudioEventKind::SpatialStop && e.kind!=PickupAudioEventKind::SpatialPlay);

    // On the following frame height is snapped to target, bfly is stopped and
    // one-shot bpop (ID 0x10) is emitted for each slot.
    (void)pickups.update(1,field,player,entities,rng,60000,3,false);
    ev=pickups.consumeAudioEvents();
    int stops=0,pops=0;
    for(const auto& e:ev){
        if(e.kind==PickupAudioEventKind::SpatialStop)++stops;
        if(e.kind==PickupAudioEventKind::SpatialPlay){ ++pops; assert(e.logicalId==0x10u); }
    }
    assert(stops==6 && pops==6);

    std::cout << "pickup audio r117 ok\n";
    return 0;
}

#include "game/entities.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

struct EntitiesTestProbe {
    static std::vector<AirEnemy>& air(Entities& e){ return e.air_; }
    static std::vector<GroundEnemy>& ground(Entities& e){ return e.ground_; }
};

int main(){
    Entities e;
    Field f;
    LegacyRandom rng;
    auto& air=EntitiesTestProbe::air(e);
    auto& ground=EntitiesTestProbe::ground(e);
    air.resize(1); ground.resize(1);
    air[0].active=true; air[0].x=20.f; air[0].y=20.f; air[0].vx=.01f; air[0].vy=0.f;
    air[0].worldX=.4625f; air[0].worldZ=.4625f; air[0].spinStep=2;
    ground[0].active=true; ground[0].x=10.f; ground[0].y=10.f; ground[0].vx=.01f; ground[0].vy=0.f;
    ground[0].respawnDelay=0.f; ground[0].worldX=.43125f; ground[0].worldZ=.43125f;
    const float gx=ground[0].x, gy=ground[0].y, delay=ground[0].respawnDelay;
    e.updateAirOnly(2,f,rng,1.0f);
    assert(air[0].x!=20.f);
    assert(ground[0].x==gx && ground[0].y==gy && ground[0].respawnDelay==delay);
    std::cout << "level intro air-only r282 PASS\n";
}

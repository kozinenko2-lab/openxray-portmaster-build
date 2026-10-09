#include "game/player.hpp"
#include "game/entities.hpp"
#include "game/field.hpp"
#include "core/legacy_random.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
struct PlayerHazardTestProbe {
    static std::vector<TrailCell>& trail(Player& p){return p.trail_;}
    static void cutting(Player& p,bool v){p.cutting_=v;}
};
struct EntitiesTestProbeR100 {
    static std::vector<GroundEnemy>& ground(Entities& e){return e.ground_;}
};
int main(){
    Player p;
    auto& t=PlayerHazardTestProbe::trail(p);
    t.push_back({10,0,20,0}); t.push_back({11,95,20,1});
    PlayerHazardTestProbe::cutting(p,true);
    p.advanceTrailPresentation(16);
    assert(t[0].progress==16); assert(t[1].progress==100);
    p.clearTrailForDeath(); assert(p.trail().empty()); assert(!p.cutting());

    Entities e; auto& g=EntitiesTestProbeR100::ground(e); g.resize(2);
    g[0].respawnDelay=0.0f; g[1].respawnDelay=1.7f;
    Field f; LegacyRandom rng;
    e.update(100,f,rng,1.0f,true);
    assert(std::fabs(g[0].respawnDelay-0.015f)<1e-6f);
    assert(std::fabs(g[1].respawnDelay-1.715f)<1e-6f);
    std::cout<<"death trail/crawler r172 PASS\n";
}

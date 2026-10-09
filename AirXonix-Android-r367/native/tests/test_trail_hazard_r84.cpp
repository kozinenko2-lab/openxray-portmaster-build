#include "game/player.hpp"
#include <cassert>

struct PlayerHazardTestProbe {
    static void seed(Player& p){
        p.trail_.clear(); p.hazardAccumulatorMs_=0;
        // Deliberately order the trail so the seed is far from the forward
        // pass's terminal pair. Coordinates test the separate 3x3 writer.
        p.trail_.push_back({10,0,10,0});
        p.trail_.push_back({20,0,20,0});
        p.trail_.push_back({30,0,30,0});
        p.trail_.push_back({40,0,40,0});
        p.trail_.push_back({50,0,50,0});
    }
    static const std::vector<TrailCell>& trail(const Player& p){return p.trail_;}
};

int main(){
    Player p; PlayerHazardTestProbe::seed(p);
    // 0x41A68D strict center-2 < coord < center+2 marks center +/-1 only.
    assert(p.markTrailHazardNear(10,10));
    auto& t=PlayerHazardTestProbe::trail(p);
    assert(t[0].hazard==1 && t[1].hazard==0);
    assert(!p.updateTrailHazard(16));
    // First 17-ms tick propagates one step toward the far end but is not yet
    // lethal because the final forward pair was still clean.
    assert(!p.updateTrailHazard(1));
    assert(t[1].hazard==1);
    assert(t[4].hazard==0);
    // Repeated exact forward/backward adjacent passes move the wave until the
    // final forward pair is non-zero, at which point death is raised.
    bool lethal=false;
    for(int i=0;i<4 && !lethal;++i) lethal=p.updateTrailHazard(17);
    assert(lethal);

    Player q; PlayerHazardTestProbe::seed(q);
    assert(!q.markTrailHazardNear(12,12)); // distance 2 is excluded exactly.
    return 0;
}

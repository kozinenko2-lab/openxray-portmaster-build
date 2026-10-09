#include "game/legacy_information_fountain.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
int main(){
    using namespace LegacyInformationFountain;
    std::array<Particle,Count> p{};
    LegacyRandom rng(1u);
    int head=0;
    emitChunk(p,head,0.f,rng);
    assert(head==16);
    const auto& q=p[16];
    assert(std::fabs(q.x-SpawnBaseX)<1e-9f);
    assert(std::fabs(q.y-SpawnY)<1e-9f);
    assert(std::fabs(q.z-SpawnZ)<1e-9f);
    // seed=1 => rand outputs 41,18467,6334; masks are 41,35,62.
    assert(std::fabs(q.vy-(BaseVy+9.f*VyRandomScale))<1e-10f);
    assert(std::fabs(q.vx-(20.f*VxzRandomScale))<1e-10f);
    assert(std::fabs(q.vz-(30.f*VxzRandomScale))<1e-10f);
    Particle n{}; n.vy=1.0e-8f;
    updateOne(n,1);
    assert(n.vy<0.f);
    std::uint32_t bits=0; std::memcpy(&bits,&n.vy,sizeof(bits));
    assert((bits&0xffu)==0u);
    return 0;
}

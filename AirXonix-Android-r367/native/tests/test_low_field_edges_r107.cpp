#include "render/legacy_field_tessellators.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace LegacyFieldTessellators;
static bool near(float a,float b,float e=1e-7f){return std::fabs(a-b)<=e;}
int main(){
    static_assert(LowEdgeY==0.0005000000237487257f);
    static_assert(LowEdgeLight==0.3499999940395355f);
    static_assert(StaticGridStep==0.0031250000465661287f);
    static_assert(StaticUvStep==0.0625f);
    std::array<std::uint8_t,W*H> f{};
    f.fill(0);
    // 41F980 is directional: only SAFE at x followed by non-SAFE at x+1.
    f[10*W+20]=Safe;
    auto h=lowHorizontalTransitions(f);
    bool found=false; for(auto e:h) if(e.x==20&&e.y==10) found=true;
    assert(found);
    // Reverse transition must not be emitted.
    f.fill(0); f[10*W+21]=Safe;
    h=lowHorizontalTransitions(f);
    for(auto e:h) assert(!(e.x==20&&e.y==10));

    // 41FB30 merges a contiguous previous-row SAFE/current-row non-SAFE run.
    f.fill(0);
    for(int x=7;x<12;++x){f[19*W+x]=Safe; f[20*W+x]=0;}
    auto runs=lowVerticalRuns(f);
    assert(runs.size()==1);
    assert(runs[0].y==20&&runs[0].x0==7&&runs[0].x1==12);
    const auto q=lowVerticalQuad(runs[0]);
    assert(near(q[0].x,gridCoord(7)) && near(q[0].z,gridCoord(20)));
    assert(near(q[1].x,gridCoord(8)) && near(q[1].z,gridCoord(21)));
    assert(near(q[2].x,gridCoord(13)) && near(q[2].z,gridCoord(21)));
    assert(near(q[3].x,gridCoord(12)) && near(q[3].z,gridCoord(20)));
    for(const auto& v:q){assert(v.y==LowEdgeY);assert(v.light==LowEdgeLight);}

    const auto qh=lowHorizontalQuad(20,10);
    assert(near(qh[0].x,gridCoord(22))&&near(qh[0].z,gridCoord(11)));
    assert(near(qh[1].x,gridCoord(21))&&near(qh[1].z,gridCoord(10)));
    assert(near(qh[2].x,gridCoord(21))&&near(qh[2].z,gridCoord(11)));
    assert(near(qh[3].x,gridCoord(22))&&near(qh[3].z,gridCoord(12)));
    assert(near(qh[0].u,gridUv(22))&&near(qh[0].v,gridUv(11)));
    std::cout<<"low field edges r107 ok\n";
}

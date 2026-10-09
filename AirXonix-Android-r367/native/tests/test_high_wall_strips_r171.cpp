#include "render/legacy_field_tessellators.hpp"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
using namespace LegacyFieldTessellators;
static bool near(float a,float b){return std::fabs(a-b)<1e-7f;}
int main(){
    static_assert(highWallLight(VerticalSafeEdges)==EdgeLightA);
    static_assert(highWallLight(HorizontalEdgeA)==EdgeLightB);
    static_assert(highWallLight(HorizontalEdgeB)==EdgeLightC);
    std::array<std::uint8_t,W*H> f{}; f.fill(Safe);
    // Three adjacent cells flip at one row boundary: 41F460 must expose one
    // endpoint-pair span, not three independent per-cell quads.
    for(int x=20;x<23;++x) f[10*W+x]=0;
    const auto runs=highVerticalRuns(f);
    bool upper=false,lower=false;
    for(const auto& r:runs){
        if(r.boundary==10&&r.a0==20&&r.a1==23) upper=true;
        if(r.boundary==11&&r.a0==20&&r.a1==23&&r.dir==EdgeDir::NonSafeToSafe) lower=true;
    }
    // r217: 0x41F460 is one-sided. SAFE->non-SAFE at boundary 10 is skipped;
    // non-SAFE->SAFE at boundary 11 is emitted.
    assert(!upper&&lower);
    assert(near(highWallLight(VerticalSafeEdges),.5f));
    assert(near(highWallLight(HorizontalEdgeA),.4f));
    assert(near(highWallLight(HorizontalEdgeB),.6f));
    // r212 direct-EXE UV contract: 0x41EE49 sets wall V=.25 and the horizontal
    // material coordinate comes from the static i*.0625 table at 0x258446C.
    static_assert(HighWallVMin==0.0f);
    static_assert(HighWallVMax==0.25f);
    static_assert(StaticUvStep==0.0625f);
    assert(near(gridUv(1),.0625f));
    assert(near(gridUv(16),1.0f));
    const auto zq=highVerticalRunQuad({11,20,23,EdgeDir::NonSafeToSafe});
    assert(zq[0].y==0.f && zq[1].y==SafeY && zq[2].y==SafeY && zq[3].y==0.f);
    assert(zq[0].u==gridUv(20) && zq[2].u==gridUv(23));
    const Edge ea{8,12,EdgeDir::SafeToNonSafe};
    const Edge eb{16,12,EdgeDir::NonSafeToSafe};
    const auto aq=highHorizontalAQuad(ea), bq=highHorizontalBQuad(eb);
    assert(aq[0].y==0.f && aq[1].y==SafeY && aq[2].y==SafeY && aq[3].y==0.f);
    assert(bq[0].y==SafeY && bq[1].y==0.f && bq[2].y==0.f && bq[3].y==SafeY);
    assert(near(aq[0].light,.4f) && near(bq[0].light,.6f));
    std::cout<<"high wall strips r218 ok\n";
}

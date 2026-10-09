#include "render/legacy_field_frontend.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

static bool near(float a,float b,float eps=1e-6f){return std::fabs(a-b)<=eps;}

int main(){
    using namespace LegacyFieldFrontend;
    static_assert(BoundaryCount==65 && CellCount==64);
    assert(Routine==0x004201E0u);
    assert(GridX==0x02584570u && GridZ==0x02584778u);
    assert(GridScaledX==0x02584674u && GridScaledZ==0x0258487Cu);
    assert(Passes.firstTessellator==0x0041F090u);
    assert(Passes.hud==0x004247E0u);
    const int expected[6]={2,0,1,0,2,3};
    for(int i=0;i<6;++i)assert(Passes.textureOrder[i]==expected[i]);

    constexpr float cx=.5f, cy=.1f, cz=.5f;
    const auto g=build(cx,cy,cz);
    assert(near(g.outerRatio,(cy+.04f)/(cy-.008f)));
    assert(near(g.fieldRatio,cy/(cy-.008f)));
    // Literal endpoint interpolation of the 64x64 playable square.
    const float left=cx+(.4f-cx)*g.fieldRatio;
    const float right=cx+(.6f-cx)*g.fieldRatio;
    assert(near(g.x.front(),left));
    assert(near(g.x.back(),right));
    assert(near(g.z.front(),left));
    assert(near(g.z.back(),right));
    // 65 boundaries must be uniformly spaced; the companion arrays are the
    // exact legacy (coord-.4)*20 material-space mapping.
    const float dx=(right-left)/64.f;
    assert(near(g.x[1]-g.x[0],dx));
    assert(near(g.x[64]-g.x[63],dx));
    assert(near(g.scaledX.front(),(g.x.front()-.4f)*20.f));
    assert(near(g.scaledX.back(),(g.x.back()-.4f)*20.f));

    // Camera-centred X/Z remain symmetric, while the outer border expands
    // farther than the playable field because of the +0.04 numerator.
    assert(near((g.outerMinX+g.outerMaxX)*.5f,cx));
    assert(g.outerMinX < g.x.front());
    assert(g.outerMaxX > g.x.back());

    // r214: literal 0x25B5B2C floor light initialized by 0x41ED90.
    assert(near(NonSafeLight,0.689116895198822f,1e-7f));

    std::cout << "field frontend r214 PASS\n";
}

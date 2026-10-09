#include "render/legacy_field_frontend.hpp"
#include "render/legacy_field_render_state.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

static bool near(float a,float b,float e=1e-6f){return std::fabs(a-b)<=e;}
int main(){
    using namespace LegacyFieldFrontend;
    assert(LegacyFieldRenderState::FloorTextureSlot==0);
    assert(LegacyFieldRenderState::BackgroundPreparedTextureSlot==1);
    assert(LegacyFieldRenderState::SafeAndBoundaryTextureSlot==2);
    const auto g=build(.5f,.1f,.5f);
    const auto v=backgroundRing(g);
    assert(v.size()==8 && BackgroundQuads.size()==4);
    assert(near(v[0].x,0.f)&&near(v[0].y,-.04f)&&near(v[0].z,0.f));
    assert(near(v[2].x,1.f)&&near(v[2].z,1.2f));
    assert(near(v[0].u,0.f)&&near(v[0].v,0.f)&&near(v[0].light,1.f));
    assert(near(v[1].v,9.6f));
    assert(near(v[1].light,0.f,2e-6f));
    assert(near(v[4].x,g.outerMinX)&&near(v[4].z,g.outerMinZ));
    assert(near(v[6].x,g.outerMaxX)&&near(v[6].z,g.outerMaxZ));
    assert(near(v[4].u,v[4].x*8.f)&&near(v[4].v,v[4].z*8.f));
    const std::array<std::uint16_t,4> q0{{0,1,5,4}};
    assert(BackgroundQuads[0]==q0);
    std::cout<<"field background r103 PASS\n";
}

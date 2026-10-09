#include <array>
#include <cassert>
#include <cmath>
#include "render/legacy_field_tessellators.hpp"
int main(){
    using namespace LegacyFieldTessellators;
    std::array<std::uint8_t,W*H> f{};
    std::array<float,16> t{};
    t[3]=0.0042f;
    f[10*W+5]=3; f[10*W+6]=3; f[10*W+7]=3;
    f[10*W+9]=3;
    f[11*W+5]=4; // inactive timer -> ignored
    auto r=captureMarkerRuns(f,t);
    assert(CaptureMarkerRuns==0x0041EE90u);
    assert(r.size()==2);
    assert(r[0].marker==3 && r[0].y==10 && r[0].x0==5 && r[0].x1==8);
    assert(r[1].x0==9 && r[1].x1==10);
    assert(std::fabs(r[0].height-0.0042f)<1e-7f);
    assert(SafeLight>0.79f && SafeLight<0.81f);
    const auto q=staticTopRunQuad(r[0].y,r[0].x0,r[0].x1,r[0].height,SafeLight);
    assert(q[0].y==r[0].height && q[3].y==r[0].height);
    assert(q[0].u==gridUv(r[0].x0));
    assert(q[2].v==gridUv(r[0].y+1));
}

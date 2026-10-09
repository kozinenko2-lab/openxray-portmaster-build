#include "render/legacy_screen_frame.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
static bool near(float a,float b,float e=1e-6f){return std::fabs(a-b)<=e;}
int main(){
    using namespace LegacyScreenFrame;
    static_assert(Builder==0x00405C30u && Submit==0x00405E40u);
    static_assert(Indices.size()==24);
    const auto v=vertices(640,480);
    assert(v.size()==8);
    assert(near(v[0].sx,0.f)&&near(v[0].sy,0.f));
    assert(near(v[1].sx,2.f)&&near(v[1].sy,2.f));
    assert(near(v[2].sx,640.f)&&near(v[2].sy,0.f));
    assert(near(v[3].sx,638.f)&&near(v[3].sy,2.f));
    assert(near(v[4].sx,640.f)&&near(v[4].sy,480.f));
    assert(near(v[5].sx,638.f)&&near(v[5].sy,478.f));
    assert(near(v[6].sx,0.f)&&near(v[6].sy,480.f));
    assert(near(v[7].sx,2.f)&&near(v[7].sy,478.f));
    for(const auto& q:v){
        assert(near(q.sz,ScreenDepth)); assert(near(q.rhw,Rhw,1e-5f));
        assert(q.diffuse==0u && q.specular==0u && q.u==0.f && q.v==0.f);
    }
    constexpr std::array<std::uint16_t,24> expected{{
        1,0,3,3,0,2, 5,3,2,5,2,4, 6,5,4,6,7,5, 6,0,7,7,0,1
    }};
    for(std::size_t i=0;i<expected.size();++i) assert(Indices[i]==expected[i]);
    std::cout<<"screen frame r223 PASS\n";
}

#include "render/legacy_field_frontend.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
static bool near(float a,float b,float e=1e-6f){return std::fabs(a-b)<=e;}
int main(){
    using namespace LegacyFieldFrontend;
    const auto v=rimVertices();
    const auto q=rimQuads();
    static_assert(RimVertexCount==76 && RimQuadCount==38 && RimRows==19);
    assert(v.size()==76 && q.size()==38);
    assert(near(v[0].x,.3874f)&&near(v[0].y,.008f)&&near(v[0].z,.3875f));
    assert(near(v[18].z,.6125f));
    assert(near(v[19].x,.4001f)&&near(v[38].x,.59900004f)&&near(v[57].x,.6126f));
    assert(near(v[0].u,.75f)&&near(v[19].u,1.f)&&near(v[38].u,5.f)&&near(v[57].u,5.25f));
    assert(near(v[0].v,.75f)&&near(v[18].v,5.25f));
    assert(near(v[0].light,.8f));
    const std::array<std::uint16_t,4> a{{0,1,20,19}},b{{38,39,58,57}},c{{19,20,39,38}},d{{36,37,56,55}};
    assert(q[0]==a);assert(q[18]==b);assert(q[36]==c);assert(q[37]==d);
    // r221: 0x40C350 consumes these as four-index polygons; the x86 topology
    // stream stores byte offsets (44 * transformed-vertex index).
    for(const auto& poly:q){
        for(const auto index:poly){
            assert(index<RimVertexCount);
            const std::uint32_t encoded=44u*static_cast<std::uint32_t>(index);
            assert(encoded%44u==0u && encoded/44u==index);
        }
    }
    std::cout<<"field rim r221 PASS\n";
}

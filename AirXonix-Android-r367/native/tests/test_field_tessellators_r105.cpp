#include "render/legacy_field_tessellators.hpp"
#include <cassert>
#include <iostream>
using namespace LegacyFieldTessellators;
int main(){
    static_assert(SafeY==0.00800000037997961f);
    static_assert(LowEdgeY==0.0005000000237487257f);
    assert(Contracts[0].address==0x0041F090u && Contracts[0].rows==64 && Contracts[0].columns==64);
    assert(Contracts[1].startOffset==65 && Contracts[1].rows==62 && Contracts[1].columns==62);
    assert(Contracts[3].startOffset==64 && Contracts[3].columns==48 && Contracts[3].rowSkip==16);
    assert(Contracts[4].startOffset==79 && Contracts[4].columns==48 && Contracts[4].rowSkip==16);
    assert(Contracts[6].rows==63 && Contracts[6].columns==64);
    std::array<std::uint8_t,W*H> f{};
    // One SAFE run on row 10.
    for(int x=5;x<9;++x) f[10*W+x]=Safe;
    auto s=safeRuns(f); bool found=false;
    for(auto r:s) if(r.y==10&&r.x0==5&&r.x1==9&&r.safe) found=true;
    assert(found);
    // Interior non-safe scan must exclude border cells and split around SAFE.
    f.fill(Safe); for(int x=20;x<24;++x) f[12*W+x]=0;
    auto n=nonSafeInteriorRuns(f); assert(n.size()==1); assert(n[0].y==12&&n[0].x0==20&&n[0].x1==24);
    // Exact transition predicates.
    f.fill(Safe); f[20*W+30]=0;
    auto v=verticalTransitions(f); bool v0=false,v1=false;
    for(auto e:v){if(e.x==30&&e.y==20) v0=true;if(e.x==30&&e.y==21) v1=true;} assert(v0&&v1);
    // r107 correction: low transition family is directional, not symmetric.
    auto h=lowHorizontalTransitions(f); bool h29=false,h30=false;
    for(auto e:h){if(e.y==20&&e.x==29)h29=true;if(e.y==20&&e.x==30)h30=true;} assert(h29&&!h30);
    auto lv=lowVerticalTransitions(f); bool l0=false,l1=false;
    for(auto e:lv){if(e.x==30&&e.y==20)l0=true;if(e.x==30&&e.y==21)l1=true;} assert(l0&&!l1);
    // r213 direct EXE: 0x41F090 and 0x41EE90 use the static UV table
    // initialized at 0x41EDE3..0x41EE12: uv[i] = i * 0.0625.
    static_assert(StaticUvStep==0.0625f);
    assert(gridUv(8)==0.5f);
    assert(gridUv(16)==1.0f);
    const Run rr{7,3,6,true};
    const auto tq=safeRunQuad(rr);
    assert(tq[0].x==gridCoord(3) && tq[0].z==gridCoord(7));
    assert(tq[1].x==gridCoord(3) && tq[1].z==gridCoord(8));
    assert(tq[2].x==gridCoord(6) && tq[2].z==gridCoord(8));
    assert(tq[3].x==gridCoord(6) && tq[3].z==gridCoord(7));
    assert(tq[0].u==gridUv(3) && tq[2].v==gridUv(8));
    std::cout<<"field tessellators r213 ok\n";
}

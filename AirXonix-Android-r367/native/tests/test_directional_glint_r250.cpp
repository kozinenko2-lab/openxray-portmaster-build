#include "render/legacy_directional_glint.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

static bool near(float a,float b,float e=2e-6f){return std::fabs(a-b)<=e;}

int main(){
    using namespace LegacyDirectionalGlint;
    static_assert(Trace::templateBuilder==0x004053B0u);
    static_assert(Trace::referenceSetter==0x004055C0u);
    static_assert(Trace::submitHelper==0x004055E0u);
    static_assert(Trace::xonixSubmit==0x00420AE0u);
    static_assert(Fan.size()==6u);
    static_assert(Fan[0][0]==6u && Fan[0][1]==0u && Fan[0][2]==5u);
    static_assert(Fan[5][0]==6u && Fan[5][1]==5u && Fan[5][2]==4u);

    constexpr float tx=.5f,ty=.02f,tz=.4f;
    const auto v=build(.6f,.10f,.3f,tx,ty,tz,Trace::xonixSize);
    for(std::size_t i=0;i<v.size();++i){
        const float dx=v[i].x-tx,dy=v[i].y-ty,dz=v[i].z-tz;
        assert(near(std::sqrt(dx*dx+dy*dy+dz*dz),Trace::xonixSize));
        assert(v[i].u==Trace::u && v[i].v==Trace::v);
        assert(v[i].light==(i==6u?1.f:0.f));
    }
    // The six triangles all share the bright source vertex and never use an
    // invented eighth/native index: this is the exact prepared fan contract.
    for(const auto& f:Fan){assert(f[0]==6u);for(auto i:f)assert(i<7u);}
    std::cout << "directional glint r250 PASS\n";
}

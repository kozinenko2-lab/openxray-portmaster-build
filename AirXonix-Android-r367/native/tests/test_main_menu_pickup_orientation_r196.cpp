#include "render/legacy_main_menu_decor.hpp"
#include <cassert>
#include <cmath>

static bool close(float a,float b){return std::fabs(a-b)<1.0e-6f;}
int main(){
    using T=LegacyMainMenuDecorationTrace;
    assert(T::menuPickupTypes[0]==5);
    assert(T::menuPickupTypes[1]==1);
    assert(T::menuPickupTypes[2]==3);
    const int spin=321;
    const auto q=T::menuPickupOrientation(0,spin);
    const auto t=T::menuPickupOrientation(1,spin);
    const auto s=T::menuPickupOrientation(2,spin);
    // 0x412C04..0x412E4A reuses exactly one matrix for the first three
    // pickups, so every coefficient must match.
    for(int i=0;i<9;++i){assert(close(q.m[i],t.m[i]));assert(close(q.m[i],s.m[i]));}
    assert(close(q.scalar,t.scalar)&&close(q.scalar,s.scalar));
    // Heart is the only independently oriented pickup.
    const auto h=T::menuPickupOrientation(3,spin);
    const auto hy=LegacyTransform::transformNormal(h,0.f,1.f,0.f);
    assert(std::fabs(hy.y-1.f)<1.0e-5f);
    bool differs=false;for(int i=0;i<9;++i)differs=differs||!close(q.m[i],h.m[i]);
    assert(differs);
    return 0;
}

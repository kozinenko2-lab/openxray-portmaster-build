#include "render/legacy_main_menu_decor.hpp"
#include <cassert>
#include <cmath>
#include <cstring>
int main(){
    using T=LegacyMainMenuDecorationTrace;
    static_assert(T::xoniDecorAtlasX0==32 && T::xoniDecorAtlasY0==24);
    static_assert(T::xoniDecorAtlasX1==40 && T::xoniDecorAtlasY1==32);
    assert(std::strcmp(T::slot6Resource,"IN2$")==0);
    constexpr float pi=3.14159265358979323846f;
    auto a=T::yawOrbit(0.f,T::slot0RadiusC);
    assert(std::fabs(a.x)<1e-7f && std::fabs(a.z-0.015f)<1e-6f);
    auto b=T::yawOrbit(pi/2.f,T::slot1Radius);
    assert(std::fabs(b.x+0.1f)<1e-6f && std::fabs(b.z)<1e-6f);
    auto c=T::yawOrbit(pi,T::slot2RadiusB);
    assert(std::fabs(c.x)<1e-6f && std::fabs(c.z+0.046f)<1e-6f);
    return 0;
}

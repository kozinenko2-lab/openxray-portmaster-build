#include "render/legacy_main_menu_decor.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
    using T=LegacyMainMenuDecorationTrace;
    static_assert(T::airborneSubtype2Radius==T::slot1Radius);
    assert(std::fabs(T::airborneSubtype2Radius+0.1f)<1e-6f);
    auto p=T::yawOrbit(0.f,T::airborneSubtype2Radius);
    assert(std::fabs(p.x)<1e-6f);
    assert(std::fabs(p.z+0.1f)<1e-6f);
    std::cout << "main menu state/radius r180 ok\\n";
}

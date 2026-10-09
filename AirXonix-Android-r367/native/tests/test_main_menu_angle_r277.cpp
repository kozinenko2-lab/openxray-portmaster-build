#include "render/legacy_main_menu_decor.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
    using T=LegacyMainMenuDecorationTrace;
    const float scale=T::angleToLegacy;
    assert(T::legacyAngle( 10.75f/scale)==10);
    assert(T::legacyAngle( 10.25f/scale)==10);
    assert(T::legacyAngle(-10.75f/scale)==-10);
    assert(T::legacyAngle(-10.25f/scale)==-10);
    assert(T::legacyAngle( 0.999f/scale)==0);
    assert(T::legacyAngle(-0.999f/scale)==0);
    std::cout << "main menu angle r277 PASS\n";
}

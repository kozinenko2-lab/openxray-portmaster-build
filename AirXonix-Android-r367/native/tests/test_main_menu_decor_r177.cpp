#include "render/legacy_main_menu_decor.hpp"
#include <cassert>
#include <cmath>
int main(){
    using T=LegacyMainMenuDecorationTrace;
    static_assert(T::slot0BaseDrawCount==5);
    static_assert(T::slot0ReflectionCount==5);
    static_assert(T::slot0FirstZRotation==-0x100);
    static_assert(T::slot0SecondZRotation==0x100);
    static_assert(T::slot1PitchOffset==-0x200);
    static_assert(T::slot2BaseDrawCount==2);
    static_assert(T::slot2ReflectionCount==2);
    assert(std::fabs(T::slot0RadiusA+0.2f)<1e-6f);
    assert(std::fabs(T::slot0RadiusB-0.2f)<1e-6f);
    assert(std::fabs(T::slot0RadiusC-0.015f)<1e-6f);
    assert(std::fabs(T::slot1Radius+0.1f)<1e-6f);
    assert(std::fabs(T::slot2RadiusA+0.016f)<1e-6f);
    assert(std::fabs(T::slot2RadiusB-0.046f)<1e-6f);
    return 0;
}

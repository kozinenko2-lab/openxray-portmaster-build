#include "render/legacy_main_menu_decor.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

static bool nearf(float a,float b,float e=1e-6f){return std::fabs(a-b)<=e;}
int main(){
    using T=LegacyMainMenuDecorationTrace;
    static_assert(!T::titleAlphaTestEnabled);
    static_assert(T::titleCameraSelectorGlobal==0x02545898u);
    static_assert(T::titleCameraSelectorInit==0u);
    T::TitleState s{};
    // Exact local motion recovered at 0x411FA4..0x412096.
    assert(nearf(T::logoTranslateX(s),0.f));
    assert(nearf(T::logoTranslateY(s),0.f));
    assert(nearf(T::logoTranslateZ(s),0.07f));
    assert(nearf(T::logoScale(s),1.f));
    assert(nearf(T::logoYaw(s),1.5707963267948966f+1.5f));
    s.reveal=1.f;s.drop=0.f;s.phase=2.5f;
    assert(nearf(T::logoTranslateX(s),-0.008299999870359898f));
    assert(nearf(T::logoTranslateY(s),-0.005799999926239252f));
    assert(nearf(T::logoTranslateZ(s),0.01f));
    assert(nearf(T::logoScale(s),0.3f));
    assert(nearf(T::logoYaw(s),0.f));
    std::cout<<"main-menu LOGO motion/opaque RGB565 contract r349 ok\n";
}

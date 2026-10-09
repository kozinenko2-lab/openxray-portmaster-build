#include "game/legacy_menu.hpp"
#include <cassert>
#include <cmath>
int main(){
    using T=LegacyMainMenuSelectorSlideTrace;
    constexpr auto k=kLegacyMainMenuSelectorSlideTrace;
    static_assert(k.targetBuildStart==0x004131FAu);
    static_assert(k.markerSubmitA==0x0041342Eu);
    static_assert(k.markerSubmitB==0x00413444u);
    assert(std::fabs(T::targetFor(4)-(-0.012000000104308128f))<1e-8f);
    float v=0.f;
    v=T::advance(v,1,10); // 10 ms * .00005 = -.0005 toward -.003
    assert(std::fabs(v-(-0.0005f))<1e-7f);
    for(int i=0;i<10;++i)v=T::advance(v,1,10);
    assert(std::fabs(v-(-0.003f))<1e-7f);
    assert(std::fabs((v+k.markerZBias)-(-0.0045f))<1e-7f);
    return 0;
}

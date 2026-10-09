#include "render/legacy_information_backdrop.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

int main(){
    using namespace LegacyInformationBackdrop;
    static_assert(QuadRoutine==0x0040E3D0u);
    static_assert(Page0QuadCall==0x00410454u);
    static_assert(Page1QuadCall==0x004106BCu);
    static_assert(Page0TextureSlot==0 && Page1TextureSlot==1);
    static_assert(UvExtent==3.0f);
    static_assert(TransitionClamp==0x7C0 && TransitionRate==3);
    assert(std::fabs(advancePhase(0,0.f,100)-0.02f)<1e-6f);
    assert(std::fabs(advancePhase(1,0.f,100)-0.03f)<1e-6f);
    assert(advanceTransition(0,100)==300);
    assert(advanceTransition(0x7b0,100)==0x7c0);
    assert(packedRgb(0x7c0)==0x007c7c7cu);
    assert(std::fabs(grayscale01(0x7c0)-(124.f/255.f))<1e-6f);
    std::cout << "information backdrop r231 PASS\n";
}

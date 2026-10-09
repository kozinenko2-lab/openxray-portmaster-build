#include "render/legacy_information_render_state.hpp"
#include <cassert>
#include <iostream>
int main(){
    using namespace LegacyInformationRenderState;
    static_assert(Routine==0x00414170u);
    static_assert(SetAlwaysCall==0x00414319u);
    static_assert(RestoreLessEqualCall==0x004143F2u);
    static_assert(FlyingMineDepthTestEnabled);
    static_assert(FlyingMineDepthWriteEnabled);
    static_assert(FlyingMineDepthAlways);
    static_assert(TextTextureSlot==5);
    static_assert(TextSharesAlwaysDepth);
    static_assert(Page0TextAdditiveBlend);
    static_assert(Page1TextAdditiveBlend && Page2TextAdditiveBlend);
    static_assert(Page0TextFlushCall==0x00410470u);
    static_assert(Page1TextFlushCall==0x004106D8u);
    static_assert(Page2TextFlushCall==0x004143CFu);
    std::cout << "information depth r227/r229 PASS\n";
}

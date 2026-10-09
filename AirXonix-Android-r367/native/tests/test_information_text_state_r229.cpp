#include "render/legacy_information_render_state.hpp"
#include <cassert>
#include <iostream>

int main(){
    using namespace LegacyInformationRenderState;
    static_assert(TextTextureSlot==5);
    static_assert(TextSharesAlwaysDepth);
    static_assert(Page0TextAdditiveBlend);
    static_assert(Page1TextAdditiveBlend);
    static_assert(Page2TextAdditiveBlend);
    static_assert(Page0TextFlushCall==0x00410470u);
    static_assert(Page1TextFlushCall==0x004106D8u);
    static_assert(Page2TextFlushCall==0x004143CFu);
    static_assert(SetAlwaysCall==0x00414319u);
    static_assert(RestoreLessEqualCall==0x004143F2u);
    std::cout << "information text state r229 PASS\n";
}

#include "render/legacy_information_render_state.hpp"
#include <iostream>

int main(){
    using namespace LegacyInformationRenderState;
    static_assert(Page1TextFlushCall==0x004106D8u);
    static_assert(Page1AlwaysAirSubtype2SubmitCall==0x0041075Fu);
    static_assert(Page1AlwaysAirSubtype0SubmitCall==0x00410791u);
    static_assert(Page1RestoreLessEqualCall==0x00410798u);
    static_assert(Page1TextFlushCall < Page1AlwaysAirSubtype2SubmitCall);
    static_assert(Page1AlwaysAirSubtype2SubmitCall < Page1AlwaysAirSubtype0SubmitCall);
    static_assert(Page1AlwaysAirSubtype0SubmitCall < Page1RestoreLessEqualCall);
    std::cout << "information page1 depth r230 PASS\n";
}

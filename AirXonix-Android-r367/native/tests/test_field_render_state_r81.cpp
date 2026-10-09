#include "render/legacy_field_render_state.hpp"
#include <cassert>
#include <iostream>
int main(){
    using namespace LegacyFieldRenderState;
    assert(BackgroundPreparedTextureSlot==1);
    assert(FloorTextureSlot==0);
    assert(SafeAndBoundaryTextureSlot==2);
    std::cout<<"r81 field material ownership ok\n";
}

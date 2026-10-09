#include "render/legacy_main_menu_decor.hpp"
#include "render/legacy_render_state.hpp"
#include <cassert>

int main(){
    using T=LegacyMainMenuDecorationTrace;
    static_assert(T::titleNormalDepth);
    static_assert(T::titleDepthWriteEnabled);

    // Direct EXE state census: 0x405A20 changes only ZFUNC. There is no
    // D3DRENDERSTATE_ZWRITEENABLE owner in the game .text, so 0x411EF0 must
    // inherit the default enabled depth-write state.
    auto s=LegacyRenderState::Initial;
    s=LegacyRenderState::setNormalDepth(s,T::titleNormalDepth);
    assert(s.depthFunction==LegacyRenderState::DepthFunction::LessEqual);
    assert(s.depthWriteEnabled==T::titleDepthWriteEnabled);
    assert(T::backgroundDepthAlways);
    assert(T::backgroundDepthWriteEnabled);
    assert(T::backgroundFrameSubmit==0x00405E40u);

    // r341: after the background helper the EXE restores LESSEQUAL and keeps
    // that state through the five M1 model submissions.
    assert(T::menuItemsDepthTestEnabled);
    assert(T::menuItemsDepthWriteEnabled);
    s=LegacyRenderState::setNormalDepth(s,true);
    assert(s.depthTestEnabled);
    assert(s.depthFunction==LegacyRenderState::DepthFunction::LessEqual);
    assert(s.depthWriteEnabled);
    return 0;
}

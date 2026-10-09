#include "render/legacy_render_state.hpp"
#include <cassert>
int main(){
    using namespace LegacyRenderState;
    auto s=Initial;
    assert(s.depthWriteEnabled);
    assert(s.depthFunction==DepthFunction::LessEqual);
    assert(!s.alphaBlendEnabled);
    assert(s.srcBlendOne&&s.dstBlendOne);
    s=setNormalDepth(s,false);
    assert(s.depthFunction==DepthFunction::Always);
    assert(s.depthWriteEnabled); // ZFUNC bypass, not glDepthMask(false)
    s=setAlpha(s,true);
    assert(s.alphaBlendEnabled&&s.srcBlendOne&&s.dstBlendOne);
    s=setAlpha(s,false);
    assert(!s.alphaBlendEnabled);
    return 0;
}

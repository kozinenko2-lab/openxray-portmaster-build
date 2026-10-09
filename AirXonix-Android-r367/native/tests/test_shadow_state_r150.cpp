#include "render/legacy_models.hpp"
#include <cassert>
#include <iostream>
int main(){
    const auto& g=LegacyModels::AirEnemyShadow;
    const auto& s=LegacyModels::AirEnemyShadowRenderState;
    assert(g.drawRoutine==0x00422DE0u);
    assert(g.confirmedTextureHandle==0);
    assert(g.builderY==0.0009f);
    assert(s.alphaBlendWrapper==0x00405A60u);
    assert(s.zFuncWrapper==0x00405A20u);
    assert(!s.alphaBlendEnabled);
    assert(s.depthTestEnabled && s.depthWriteEnabled);
    assert(s.legacyZFuncValue==1 && s.d3dZFunc==4);
    std::cout << "0x422DE0 opaque LEQUAL depth-writing shadow state ok\n";
}

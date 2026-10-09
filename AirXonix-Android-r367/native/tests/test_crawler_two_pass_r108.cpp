#include "render/legacy_models.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
    using namespace LegacyModels;
    const auto& g=GroundEnemyPresentation;
    assert(g.firstPass==0x00418840u);
    assert(g.secondPass==0x00418A90u);
    assert(g.firstModelLoad==0x00418864u);
    assert(g.secondModelLoad==0x00418AB4u);
    assert(g.preparedModel==0x0257F5B8u);
    assert(g.submitRoutine==0x0041ECC0u);
    assert(g.postTransformYOffsetGlobal==0x02585A64u);
    assert(std::fabs(g.firstTransformYOffset-0.005f)<1e-7f);
    assert(std::fabs(g.firstPostTransformYOffset-0.008f)<1e-7f);
    assert(std::fabs(g.secondTransformYOffset-0.013f)<1e-7f);
    // r348: the two 0x41ECC0 outputs are fixed floor planes, not two bodies.
    assert(std::fabs(g.projectionPlaneY-0.0009f)<1e-7f);
    assert(std::fabs(g.projectionSkewX-0.5f)<1e-7f && std::fabs(g.projectionSkewZ-0.5f)<1e-7f);
    assert(std::fabs(g.projectionUvScale-20.f)<1e-7f);
    assert(std::fabs(g.firstProjectionY()-0.0089f)<1e-7f);
    assert(std::fabs(g.secondProjectionY()-0.0009f)<1e-7f);
    // The pink visible body is a third, normal texture-3 0x40C350 submit.
    assert(g.bodyLoop==0x0041A3E7u && g.bodySubmitCall==0x0041A415u);
    assert(g.bodySubmitRoutine==0x0040C350u);
    assert(g.bodyRecord==0x0254D8F8u);
    assert(std::fabs(g.bodyYOffset-0.013f)<1e-7f);
    assert(g.bodyTextureSlot==3);
    assert(g.firstTextureSlot==2 && g.secondTextureSlot==0);
    assert(g.firstDepthAlways && g.secondDepthLessEqual);
    assert(std::fabs(g.firstLightScalar-0.5f)<1e-7f);
    assert(std::fabs(g.secondLightScalar-0.35f)<1e-7f);
    assert(g.lightScalarGlobal==0x025B5B30u && g.secondLightScalarGlobal==0x025849A8u);
    assert(!g.perInstanceRotation);
    assert(!g.extraTexture3Pass && !g.extraReflectionPass);
    std::cout << "crawler body + floor projections r348 ok\n";
}

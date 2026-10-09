#include "render/legacy_field_render_state.hpp"
#include <cassert>
#include <iostream>
int main(){
    using namespace LegacyFieldRenderState;
    assert(FloorTextureSlot==0);
    assert(BackgroundPreparedTextureSlot==1);
    assert(SafeAndBoundaryTextureSlot==2);
    assert(FrontLogoTextureSlot==3);
    assert(ShadowTextureSlot==0);
    assert(BackgroundUsesZAlways);
    assert(FloorUsesZAlways);
    assert(BoundariesUseTexture2);
    assert(ZAlwaysKeepsDepthTestEnabled);
    assert(ZAlwaysKeepsDepthWritesEnabled);

    constexpr std::array<FieldPass,10> expected{{
        FieldPass::PreparedRim,
        FieldPass::SafeTop,
        FieldPass::CrawlerPass1,
        FieldPass::NonSafeFloor,
        FieldPass::CrawlerPass2,
        FieldPass::BackgroundRing,
        FieldPass::LowEdges,
        FieldPass::HighWalls,
        FieldPass::CaptureMarkers,
        FieldPass::FrontLogo
    }};
    static_assert(PassOrder.size()==expected.size());
    for(std::size_t i=0;i<expected.size();++i) assert(PassOrder[i].pass==expected[i]);

    assert(PassOrder[0].depth==DepthCompare::Always);
    assert(PassOrder[1].depth==DepthCompare::Always);
    assert(PassOrder[2].depth==DepthCompare::Always);
    assert(PassOrder[3].depth==DepthCompare::Always);
    assert(PassOrder[4].depth==DepthCompare::LessEqual);
    assert(PassOrder[5].depth==DepthCompare::Always);
    assert(PassOrder[6].depth==DepthCompare::LessEqual);
    assert(PassOrder[7].depth==DepthCompare::LessEqual);
    assert(PassOrder[8].depth==DepthCompare::LessEqual);
    assert(PassOrder[9].depth==DepthCompare::Always);

    std::cout << "field_render_state r219 PASS\n";
}

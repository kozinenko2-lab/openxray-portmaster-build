#include "render/legacy_theme.hpp"
#include "game/legacy_records_transition.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

int main(){
    const auto& d=kLegacyRecordsDecorationTrace;
    assert(kLegacyRecordsTextGridTrace.texture5Select==0x0040FAEFu);
    assert(kLegacyRecordsTextGridTrace.alphaEnable==0x0040FAF8u);
    assert(kLegacyRecordsTextGridTrace.renderCall==0x0040FB05u);
    assert(kLegacyRecordsTextGridTrace.alphaDisable==0x0040FB0Bu);
    assert(d.texture3Select==0x0040FB10u);
    assert(d.pickupSubmitA==0x0040FB7Eu);
    assert(d.pickupSubmitB==0x0040FC07u);
    assert(d.restoreLessEqual==0x0040FC33u);
    assert(d.crawlerSubmitFirst==0x0040FD22u);
    assert(d.finalAlphaEnable==0x0040FF29u);
    assert(d.finalTexture6Select==0x0040FF30u);
    assert(d.finalLayerSubmit==0x0040FF37u);
    assert(std::fabs(LegacyRecordsTransition::modelLightScale(0x7c0)-248.f/255.f)<1e-7f);
    std::cout << "records depth r235 PASS\n";
}

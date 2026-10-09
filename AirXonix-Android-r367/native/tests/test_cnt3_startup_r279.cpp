#include "render/legacy_cnt3_trace.hpp"
#include "render/legacy_cinematic_family_trace.hpp"
#include <cassert>
#include <iostream>
int main(){
    assert(kLegacyCnt3Trace.hasRuntimeConsumer);
    assert(kLegacyCnt3Trace.atlas==4 && kLegacyCnt3Trace.atlasY==220);
    assert(kLegacyCnt3Trace.digitBuilder==0x0040EE40u);
    assert(kLegacyCnt3Trace.submit==0x0041DB58u);
    assert(LegacyCinematicFamily::Slots[1].prepared==0x0257F5D0u);
    assert(std::string(LegacyCinematicFamily::Slots[1].resource)=="LEV2");
    std::cout << "cnt3 startup r279 PASS\n";
}

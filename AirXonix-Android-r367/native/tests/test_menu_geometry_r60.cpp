#include "render/legacy_theme.hpp"
#include <cassert>
#include <cmath>

int main(){
    const auto a=makeLegacyMenuLayer0(.25f);
    assert(a.baseAddress==0x02545710u);
    assert(a.topologyByteOffsets[0]==0u && a.topologyByteOffsets[3]==132u);
    assert(std::fabs(a.vertices[0].z-.09f)<1e-7f);
    assert(std::fabs(a.vertices[0].v-.25f)<1e-7f);
    assert(std::fabs(a.vertices[1].v-8.25f)<1e-6f);
    assert(std::fabs(a.vertices[2].u-16.f)<1e-7f);
    assert(std::fabs(a.vertices[0].light-1.f)<1e-7f);
    assert(std::fabs(a.vertices[1].light-0.f)<1e-7f);

    const auto b=makeLegacyMenuLayer1(.75f);
    assert(b.baseAddress==0x025458A8u);
    assert(std::fabs(b.vertices[0].v-.75f)<1e-7f);
    assert(std::fabs(b.vertices[1].v-4.75f)<1e-6f);
    assert(std::fabs(b.vertices[1].y-.5f)<1e-7f);
    assert(std::fabs(b.vertices[2].u-4.f)<1e-7f);
    assert(std::fabs(b.vertices[0].light-0.f)<1e-7f);
    assert(std::fabs(b.vertices[1].light-1.f)<1e-7f);

    const auto& m=kLegacyMainMenuItemPresentationTrace;
    assert(m.modelBuilder==0x004229B0u);
    assert((m.modelSlots==std::array<int,5>{{0,1,2,3,5}}));
    assert(std::fabs(m.zPositions[0]+.0015f)<1e-7f);
    assert(std::fabs(m.zPositions[4]+.0135f)<1e-7f);
    assert(std::fabs(m.selectedScale-1.15f)<1e-7f);
    assert(std::fabs(m.unselectedBrightness-.6f)<1e-7f);
    assert(std::fabs(LegacyMainMenuItemPresentationTrace::approach(1.f,1.15f,.1f)-1.1f)<1e-6f);
    assert(std::fabs(LegacyMainMenuItemPresentationTrace::approach(1.f,.6f,.2f)-.8f)<1e-6f);

    const auto& c=kLegacyRecordsBackgroundColourTrace;
    assert(c.packedRgb(0)==0x00000000u);
    assert(c.packedRgb(0x7c0)==0x007c7c7cu);
    return 0;
}

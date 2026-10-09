#include "render/legacy_theme.hpp"
#include <cassert>
#include <cmath>

int main(){
    const auto& m=kLegacyMenuBackgroundAnimationTrace;
    assert(m.routine==0x00411D60u);
    assert(m.texture0Select==0x00411EA6u && m.texture0Submit==0x00411EBFu);
    assert(m.texture1Select==0x00411EC4u && m.texture1Submit==0x00411EDFu);
    assert(std::fabs(m.submitX+1.f)<1e-7f);
    assert(std::fabs(m.submitY+0.05f)<1e-7f);
    assert(std::fabs(m.submitZ+0.05f)<1e-7f);
    assert(std::fabs(LegacyMenuBackgroundAnimationTrace::advancePrimary(.95f,200)-.01f)<2e-5f);
    const float p=LegacyMenuBackgroundAnimationTrace::advancePrimary(0.f,1000);
    assert(std::fabs(p-.3f)<2e-5f);
    const float s=LegacyMenuBackgroundAnimationTrace::advanceSecondary(0.f,p);
    assert(std::fabs(s-.99917f)<2e-5f);

    const auto& r=kLegacyRecordsBackgroundTrace;
    assert(r.recordsRoutine==0x0040F160u);
    assert(r.texture1Select==0x0040FA74u);
    assert(r.quadCall==0x0040FAE7u && r.quadRoutine==0x0040E3D0u);
    assert(std::fabs(r.extentX-3.f)<1e-7f && std::fabs(r.extentY-3.f)<1e-7f);
    assert(std::fabs(r.uvInset0-.99f)<1e-6f);
    assert(std::fabs(r.uvInset1-1.01010096f)<1e-6f);
    assert(std::fabs(LegacyRecordsBackgroundTrace::advancePhase(.9f,400)-.02f)<2e-5f);
    return 0;
}

#include <cassert>
#include <cmath>
#include "game/legacy_settings_visual_trace.hpp"
#include "game/legacy_settings_trace.hpp"
int main(){
    static_assert(kLegacySettingsVisualTrace.loopBegin==0x00413D02u);
    assert(std::fabs(kLegacySettingsVisualTrace.rowZ(0)-(-.004f))<1e-7f);
    assert(std::fabs(kLegacySettingsVisualTrace.rowZ(1)-(-.0068f))<1e-7f);
    assert(std::fabs(kLegacySettingsVisualTrace.rowZ(2)-(-.0096f))<1e-7f);
    assert(std::fabs(kLegacySettingsVisualTrace.speechZ-(-.0124f))<1e-7f);
    assert(std::fabs(kLegacySettingsVisualTrace.knobX(0.f)-.004f)<1e-7f);
    assert(std::fabs(kLegacySettingsVisualTrace.knobX(500.f)-.009f)<1e-7f);
    assert(std::fabs(kLegacySettingsVisualTrace.knobX(1000.f)-.014f)<1e-7f);
    assert(std::fabs(LegacySettingsTrace::selectorTarget(1)-(-.0028f))<1e-7f);
    assert(std::fabs(LegacySettingsTrace::selectorTarget(5)-(-.014f))<1e-7f);
    assert(std::fabs(LegacySettingsTrace::cameraZ(0.f)-(-.003f))<1e-7f);
    assert(std::fabs(LegacySettingsTrace::cameraZ(-.0028f)-(-.003616f))<1e-7f);
}

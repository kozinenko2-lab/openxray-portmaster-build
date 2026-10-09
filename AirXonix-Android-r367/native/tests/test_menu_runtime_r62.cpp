#include "game/legacy_menu.hpp"
#include "render/legacy_theme.hpp"
#include <array>
#include <cassert>
#include <cmath>

static bool near(float a,float b,float e=1e-6f){return std::fabs(a-b)<e;}

int main(){
    const auto& t=kLegacyMenuThemeSelectorTrace;
    assert(t.routine==0x00422F40u);
    assert(t.table==0x00441830u);
    std::array<std::uint32_t,8> usage{{0,0,0,0,0,0,0,0}};
    // Legacy scan order is 1..7,0. rand&7 == 0 selects the first minimum => 1.
    assert(t.select(usage,0)==1u);
    assert(usage[1]==1u);
    // With theme 1 no longer minimal, the first minimum in the same scan is 2.
    assert(t.select(usage,0)==2u);
    // If only theme 0 is minimum, every wanted rank eventually wraps to 0.
    usage={{0,1,1,1,1,1,1,1}};
    assert(t.select(usage,7)==0u);
    assert(usage[0]==1u);

    float b=1.0f,s=1.0f;
    b=LegacyMainMenuRuntime::approachBrightness(b,false,100); // -0.2
    s=LegacyMainMenuRuntime::approachScale(s,true,100);       // +0.1
    assert(near(b,0.8f));
    assert(near(s,1.1f));
    b=LegacyMainMenuRuntime::approachBrightness(b,false,1000);
    s=LegacyMainMenuRuntime::approachScale(s,true,1000);
    assert(near(b,0.6f));
    assert(near(s,1.15f));

    const auto& d=kLegacyRecordsDecorationTrace;
    assert(d.texture3Select==0x0040FB10u);
    assert(d.pickupModel0==0x025849C0u);
    assert(d.pickupPrepared0==0x025835E4u);
    assert(d.pickupMirrorCount==2);
    assert(near(d.pickupX,0.035f));
    assert(near(d.pickupY,0.048f));
    assert(near(d.pickupZ,0.08f));
    assert(d.crawlerModel==0x0257F5B8u && d.crawlerCount==6);
    assert(d.companionModel==0x02585A5Cu && d.companionCount==6);
    assert(d.finalAlphaEnable==0x0040FF29u);
    assert(d.finalTexture6Select==0x0040FF30u);
    return 0;
}

#include <cassert>
#include <vector>
#include "render/legacy_cnt3_trace.hpp"
#include "render/legacy_hud.hpp"

int main(){
    static_assert(kLegacyCnt3Trace.hasRuntimeConsumer);
    assert(kLegacyCnt3Trace.atlas==4);
    assert(kLegacyCnt3Trace.atlasX==0 && kLegacyCnt3Trace.atlasY==220);

    std::vector<LegacyHudSprite> digits;
    LegacyHud::appendNumber(digits,LegacyHudAtlas::Atlas4,123,3,0.f,0.f,false);
    assert(digits.empty());

    LegacyHudState s{}; s.screen=LegacyHudScreen::InterLevel;
    const auto inter=LegacyHud::compose(s);
    assert(!inter.empty());
    // r279: CNT3 is consumed by the 3-D startup presentation, not by HUD.
    for(const auto& q:inter) assert(!(q.atlas==LegacyHudAtlas::Atlas4 && q.sy>=220));
    return 0;
}

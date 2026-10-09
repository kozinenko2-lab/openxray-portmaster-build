#include "render/legacy_atlas.hpp"
#include "game/legacy_effects.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

int main(){
    assert(kLegacyShadRect.atlas==3);
    assert(kLegacyShadRect.x==0 && kLegacyShadRect.y==64);
    assert(kLegacyShadRect.w==16 && kLegacyShadRect.h==16);
    assert(!kLegacyShadHasDrawConsumer);

    const auto& t=kLegacyTimeoutAuxiliary;
    assert(t.auxiliarySlot==5);
    assert(t.textureHandle==7);
    assert(t.resource=="TOU2");
    assert(t.atlas==7 && t.atlasX==0 && t.atlasY==144);
    assert(t.sourceWidth==256 && t.sourceHeight==48);
    assert(std::fabs(t.meshU0-0.501953125f)<1e-9f);
    assert(std::fabs(t.meshU1-0.998046875f)<1e-9f);
    assert(std::fabs(t.meshV0-0.564453125f)<1e-9f);
    assert(std::fabs(t.meshV1-0.748046875f)<1e-9f);
    std::cout << "r156 SHAD orphan + TOU2 timeout identity confirmed\
";
}

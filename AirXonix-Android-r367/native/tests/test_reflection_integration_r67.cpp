#include "render/builtin_resources.hpp"
#include "render/legacy_atlas_runtime.hpp"
#include "render/legacy_reflection.hpp"
#include <array>
#include <cassert>
#include <cmath>

static bool near(float a,float b,float e=1e-6f){return std::fabs(a-b)<=e;}

int main(){
    // Exact pickup call-site intensities recovered from 0x417E12..0x417FE6.
    constexpr std::array<float,6> queued{{0.39f,0.57f,0.18f,0.39f,0.57f,0.57f}};
    for(std::size_t i=0;i<LegacyReflection::PickupRequestedIntensity.size();++i)
        assert(near(LegacyReflection::queuedBrightness(LegacyReflection::PickupRequestedIntensity[i]),queued[i]));

    LegacyAtlasImage tex;
    assert(BuiltinResources::buildTexture("1111",tex));
    assert(tex.width==128 && tex.height==128);
    bool hasTransparent=false,hasHighlight=false;
    for(std::size_t i=0;i<tex.rgba.size();i+=4){
        hasTransparent |= tex.rgba[i+3]==0;
        hasHighlight |= tex.rgba[i+3]>128 && tex.rgba[i+2]>200;
    }
    assert(hasTransparent);
    assert(hasHighlight);
    return 0;
}

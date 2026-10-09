#include "render/builtin_resources.hpp"
#include "render/legacy_atlas.hpp"
#include "render/legacy_atlas_runtime.hpp"
#include <cassert>
#include <iostream>
#include <string>

static const LegacyAtlasSourceExtent* extent(const char* n){
    for(const auto& e:kLegacyHudSourceExtents) if(std::string(e.fourcc)==n) return &e;
    return nullptr;
}
int main(){
    for(const char* n: {"SHAD","PAUS","SCOR","PERC"}){
        LegacyAtlasImage img;
        assert(BuiltinResources::buildTexture(n,img));
        const auto* e=extent(n); assert(e);
        assert(img.width==e->width); assert(img.height==e->height);
    }
    LegacyAtlasImage shad; BuiltinResources::buildTexture("SHAD",shad);
    assert(shad.width==16 && shad.height==16);
    std::cout << "resource-free HUD/SHAD extents match original BMPPACK\n";
}

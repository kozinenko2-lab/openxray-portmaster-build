#include "render/legacy_atlas_runtime.hpp"
#include <cassert>
#include <string>
int main(){
    LegacyAtlasImage image;std::string error;
    assert(LegacyAtlasRuntime::loadTexture("/definitely/missing","VOL0",nullptr,image,&error));
    assert(image.width==64&&image.height==64&&image.rgba.size()==64u*64u*4u);
    assert(LegacyAtlasRuntime::loadTexture("/definitely/missing","BALL",nullptr,image,&error));
    assert(image.width==32&&image.height==32);
    assert(LegacyAtlasRuntime::buildGameplayAtlas3("/definitely/missing",image,&error,nullptr));
    assert(image.width==256&&image.height==256);
    assert(LegacyAtlasRuntime::buildUiAtlas4("/definitely/missing",image,&error,nullptr));
    assert(LegacyAtlasRuntime::buildAuxAtlas7("/definitely/missing",image,&error,nullptr));
    assert(LegacyAtlasRuntime::buildMenuAtlas4M1("/definitely/missing",image,&error,nullptr));
    assert(LegacyAtlasRuntime::buildMenuAtlas4M2("/definitely/missing",image,&error,nullptr));
    return 0;
}

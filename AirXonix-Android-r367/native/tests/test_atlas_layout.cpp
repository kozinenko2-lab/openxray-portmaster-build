#include "render/legacy_atlas.hpp"
#include <cassert>
#include <cstring>
#include <iostream>

namespace {
template <class Range>
const LegacyAtlasEntry* findEntry(const Range& r,const char* name){
    for(const auto& e:r) if(std::strcmp(e.fourcc,name)==0) return &e;
    return nullptr;
}
}

int main(){
    assert(kLegacyAtlasCopier.copierAddress==0x004058F0u);
    assert(kLegacyAtlasCopier.resolverAddress==0x00409D80u);
    assert(kLegacyAtlasCopier.recordSize==16u && kLegacyAtlasCopier.sentinelFourccZero);
    assert(kLegacyShadRect.atlas==3 && kLegacyShadRect.x==0 && kLegacyShadRect.y==64);
    assert(kLegacyShadRect.w==16 && kLegacyShadRect.h==16);
    if(kLegacyAtlasConstructors.size()!=4)return 90;
    if(kLegacyAtlasConstructors[2].address!=0x00423C30u || kLegacyAtlasConstructors[2].family!=LegacyAtlasConstructorFamily::MenuM1)return 91;
    if(kLegacyAtlasConstructors[3].address!=0x00423EE0u || kLegacyAtlasConstructors[3].family!=LegacyAtlasConstructorFamily::MenuM2)return 92;
    if(kLegacyAtlasConstructors[2].rebuiltAtlas!=4 || kLegacyAtlasConstructors[3].rebuiltAtlas!=4)return 93;
    const auto* cnt3=findEntry(kLegacyGameplayAtlasBase,"CNT3");
    const auto* lev2=findEntry(kLegacyGameplayAtlasBase,"LEV2");
    const auto* shad=findEntry(kLegacyGameplayAtlasBase,"SHAD");
    assert(cnt3 && cnt3->atlas==4 && cnt3->x==0 && cnt3->y==220);
    assert(lev2 && lev2->atlas==7 && lev2->x==0 && lev2->y==192);
    assert(shad && shad->atlas==3 && shad->x==0 && shad->y==64);
    auto extent=[](const char* name)->const LegacyAtlasSourceExtent*{
        for(const auto& e:kLegacyHudSourceExtents) if(std::strcmp(e.fourcc,name)==0) return &e;
        return nullptr;
    };
    assert(extent("CNT3") && extent("CNT3")->width==256 && extent("CNT3")->height==36);
    assert(extent("LEV2") && extent("LEV2")->width==256 && extent("LEV2")->height==64);
    assert(extent("PAUS") && extent("PAUS")->width==64 && extent("PAUS")->height==24);

    // GAME/gam2 belong to gameplay/HUD variants, not the dedicated M1/M2
    // screen constructors. This regression guard prevents r29's temporary
    // main-menu semantic from silently returning.
    const auto* game=findEntry(kLegacyGameplayUiVariantA,"GAME");
    const auto* gam2=findEntry(kLegacyGameplayUiVariantB,"gam2");
    assert(game && game->atlas==4 && game->x==0 && game->y==128);
    assert(gam2 && gam2->atlas==4 && gam2->x==0 && gam2->y==128);
    const auto* m101=findEntry(kLegacyAtlasSetM1,"M101");
    const auto* m102=findEntry(kLegacyAtlasSetM1,"M102");
    const auto* m106=findEntry(kLegacyAtlasSetM1,"M106");
    assert(m101 && m101->atlas==4 && m101->y==0);
    assert(m102 && m102->atlas==4 && m102->y==48);
    assert(m106 && m106->atlas==4 && m106->y==192);
    const auto* m250=findEntry(kLegacyAtlasSetM2,"M250");
    const auto* m260=findEntry(kLegacyAtlasSetM2,"M260");
    const auto* m230=findEntry(kLegacyAtlasSetM2,"M230");
    assert(m250 && m250->atlas==4 && m250->y==0);
    assert(m260 && m260->atlas==4 && m260->y==42);
    assert(m230 && m230->atlas==4 && m230->y==210);
    assert(findEntry(kLegacyAtlasSetM1,"GAME") == nullptr);
    assert(findEntry(kLegacyAtlasSetM2,"GAME") == nullptr);
    std::cout << "atlas ownership/layout anchors ok\n";
}

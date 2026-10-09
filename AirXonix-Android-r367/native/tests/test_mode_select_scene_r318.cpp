#include "render/legacy_theme.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
    const auto& t=kLegacyModeSelectPresentationTrace;
    assert(t.airborneAngle(0)==0);
    assert(t.airborneAngle(2049)==1);
    assert(t.crawlerXAngle()==0x200);
    assert(std::fabs(t.slot0Z-0.005000000353902578f)<1e-9f);
    assert(t.crawlerZAngle(0x805)==5);
    assert(std::fabs(t.crawlerReflectionIntensity()-0.75f)<1e-7f);
    assert(std::fabs(t.airborneZ(0)-(-0.003000000026077032f))<1e-7f);
    assert(std::fabs(t.airborneZ(4)-(-0.01000000024214387f))<3e-7f);
    assert(std::fabs(t.crawlerScale(0)-0.15000000596046448f)<1e-7f);
    assert(t.crawlerScale(314)>0.199f);
    assert(t.crawlerScale(942)<0.101f);
    assert(t.footerRed(0)==159);
    assert(t.footerPackedRgb(0)==0x009f0000u);
    assert(t.footerRed(512)==190);
    assert(t.textPaletteLevel(0)==0);
    assert(t.textPaletteLevel(0x7c0)==31);
    assert(t.fadeTextRgb(0xffffffu,0)==0u);
    assert(t.fadeTextRgb(0xffffffu,0x7c0)==0xffffffu);
    assert(t.fadeTextRgb(0x008fffu,0x3e0)==0x00457bu);
    std::cout<<"mode select scene r318 PASS\
";
}

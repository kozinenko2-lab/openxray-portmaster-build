#include "render/legacy_hud.hpp"
#include "render/legacy_theme.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

namespace {
bool sameUv(const LegacyHudSprite& q,unsigned char ch){
    const auto uv=kLegacyFnt4GlyphTrace.uvForGlyph(kLegacyFnt4GlyphTrace.glyphIndex(ch));
    return q.u0==uv[0] && q.v0==uv[1] && q.u1==uv[2] && q.v1==uv[3];
}
}

int main(){
    LegacyHudState s{};
    s.screen=LegacyHudScreen::Records;
    const std::string name="PLAYER ONE      "; // exactly 16 bytes
    assert(name.size()==16u);
    for(std::size_t i=0;i<16;++i)s.recordsNames[0][i]=name[i];
    s.recordsValues[0]=123456u;
    s.recordsValues[1]=7u;

    const auto q=LegacyHud::compose(s);
    // Fixed prefix: heading + separators + two 11-cell mode labels.
    constexpr std::size_t firstRow=87u;
    for(std::size_t i=0;i<16;++i){
        const auto& e=q[firstRow+i];
        assert(e.x==float((7+int(i))*16) && e.y==3.f*32.f);
        assert(sameUv(e,static_cast<unsigned char>(name[i])));
    }
    // Leader stays at cols 23..32; six-digit score is right-aligned at 27..32.
    constexpr std::size_t firstScore=firstRow+26u;
    const std::string score="123456";
    for(std::size_t i=0;i<score.size();++i){
        const auto& e=q[firstScore+i];
        assert(e.x==float((27+int(i))*16) && e.y==3.f*32.f);
        assert(e.r==1.f && e.g==1.f && e.b==0.f);
        assert(sameUv(e,static_cast<unsigned char>(score[i])));
    }
    // Row 4 score 7 starts at col 32, proving exact 33-len alignment.
    const std::size_t secondRow=firstScore+score.size();
    // Row widths are data-dependent now: 16 name + 10 leader + score digits.
    assert(q[secondRow+26u].x==32.f*16.f);
    assert(sameUv(q[secondRow+26u],static_cast<unsigned char>('7')));

    std::cout << "records values r238 PASS\n";
}

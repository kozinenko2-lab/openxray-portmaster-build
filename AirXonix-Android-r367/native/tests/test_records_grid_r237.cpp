#include "render/legacy_hud.hpp"
#include "render/legacy_theme.hpp"
#include "game/legacy_records_transition.hpp"
#include <array>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>

namespace {
bool sameUv(const LegacyHudSprite& q, unsigned char ch){
    const auto uv=kLegacyFnt4GlyphTrace.uvForGlyph(kLegacyFnt4GlyphTrace.glyphIndex(ch));
    return q.u0==uv[0] && q.v0==uv[1] && q.u1==uv[2] && q.v1==uv[3];
}
void expectColour(const LegacyHudSprite& q,float r,float g,float b){
    assert(q.r==r && q.g==g && q.b==b);
}
}

int main(){
    static const std::array<std::string,5> expectedModes{{
        "  \xCF\xD0\xCE\xD1\xD2\xC0\xDF  ",
        " \xCA\xCB\xC0\xD1\xD1\xC8\xCA\xC0  ",
        "  \xCC\xCE\xC4\xC5\xD0\xCD   ",
        "   \xD5\xC0\xD0\xC4    ",
        "  \xDD\xCA\xD1\xD2\xD0\xC8\xCC  "
    }};

    for(int mode=0;mode<5;++mode){
        LegacyHudState s{};
        s.screen=LegacyHudScreen::Records;
        s.recordsMode=mode;
        // r325 supersedes the old hard-coded mode-name assumption: compose()
        // receives the already decoded SOUNDINF name. Feed the recovered
        // 11-cell field explicitly so this r237 test stays focused on the
        // grid geometry while still exercising the later resource-backed path.
        s.recordsModeName=expectedModes[std::size_t(mode)];
        const auto q=LegacyHud::compose(s);

        // r237: exact default grid = 13 heading + 52 separators +
        // 22 mode-field cells + 10*(16 name + 10 leader + 1 score).
        assert(q.size()==357u);

        // r327 refined r237: the two edge cells stay red while the eleven
        // inner heading cells carry the recovered travelling colour wave.
        for(std::size_t i=0;i<13;++i){
            assert(q[i].x==float((14+int(i))*16));
            assert(q[i].y==32.f);
            if(i==0u || i==12u){
                expectColour(q[i],1.f,0.f,0.f);
            }else{
                const std::uint32_t c=LegacyRecordsTransition::headingWaveRgb(s.recordsHeadingPhase,int(i-1u));
                expectColour(q[i],
                    float((c>>16)&0xffu)/255.f,
                    float((c>>8)&0xffu)/255.f,
                    float(c&0xffu)/255.f);
            }
        }

        // Separators: 26 blue cells at rows 2 and 13, col 7.
        for(std::size_t i=0;i<26;++i){
            const auto& top=q[13+i];
            const auto& bottom=q[39+i];
            assert(top.x==float((7+int(i))*16) && top.y==64.f);
            assert(bottom.x==float((7+int(i))*16) && bottom.y==416.f);
            expectColour(top,0.f,0.f,1.f);
            expectColour(bottom,0.f,0.f,1.f);
        }

        // 0x40F100 yields exactly eleven cells centered in the field.
        assert(kLegacyRecordsTextGridTrace.modeLabelLength==11);
        assert(expectedModes[std::size_t(mode)].size()==11u);
        for(std::size_t i=0;i<11;++i){
            const auto& top=q[65+i];
            const auto& bottom=q[76+i];
            assert(top.x==float((15+int(i))*16) && top.y==64.f);
            assert(bottom.x==float((15+int(i))*16) && bottom.y==416.f);
            expectColour(top,1.f,1.f,1.f);
            expectColour(bottom,1.f,1.f,1.f);
            assert(sameUv(top,static_cast<unsigned char>(expectedModes[std::size_t(mode)][i])));
            assert(sameUv(bottom,static_cast<unsigned char>(expectedModes[std::size_t(mode)][i])));
        }

        // Ten default rows. Name+leader are contiguous cyan cols 7..32;
        // the yellow zero is right-aligned to scoreRightEdgeColumn=33.
        std::size_t base=87;
        for(int row=3;row<=12;++row){
            for(int i=0;i<26;++i){
                const auto& e=q[base+std::size_t(i)];
                assert(e.x==float((7+i)*16) && e.y==float(row*32));
                expectColour(e,0.f,1.f,1.f);
                assert(sameUv(e,static_cast<unsigned char>('.')));
            }
            const auto& score=q[base+26];
            assert(score.x==32.f*16.f && score.y==float(row*32));
            expectColour(score,1.f,1.f,0.f);
            assert(sameUv(score,static_cast<unsigned char>('0')));
            base+=27;
        }
        assert(base==q.size());
    }

    std::cout << "records grid r237 PASS\n";
}

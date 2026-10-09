#include "game/legacy_records_transition.hpp"
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
bool gray(const LegacyHudSprite& q,int g){
    const float v=float(g)/255.f;
    return q.r==v && q.g==v && q.b==v;
}
}

int main(){
    using namespace LegacyRecordsTransition;
    assert(advanceNamePulsePhase(0,100)==500);
    assert(advanceNamePulsePhase(2000,20)==52);
    assert(namePulseByte(0)==254);
    assert(namePulseByte(512)==191);
    assert(namePulseByte(1024)==128);
    assert(namePulseByte(1536)==191);

    LegacyHudState s{};
    s.screen=LegacyHudScreen::Records;
    s.recordsNameEntry=true;
    s.recordsCandidateRow=2;
    s.recordsTypedNameLength=2;
    s.recordsNamePulseByte=128;
    s.recordsNames[2].fill('.');
    s.recordsNames[2][0]='A'; s.recordsNames[2][1]='b';
    s.recordsValues[2]=12345u;
    const auto q=LegacyHud::compose(s);

    // The four overlay calls are appended after the ordinary Records grid:
    // r326 adds the original 16-glyph row-14 name-entry prompt after the
    // 16 name + 10 leader + 5 score + 1 cursor overlay.
    assert(q.size()>=48u);
    const std::size_t base=q.size()-48u;
    for(int i=0;i<16;++i){
        const auto& e=q[base+static_cast<std::size_t>(i)];
        assert(e.x==float((7+i)*16) && e.y==5.f*32.f);
        assert(e.r==1.f && e.g==1.f && e.b==1.f);
    }
    const std::size_t scoreBase=base+26u;
    const std::string score="12345";
    for(std::size_t i=0;i<score.size();++i){
        const auto& e=q[scoreBase+i];
        assert(e.x==float((28+int(i))*16) && e.y==5.f*32.f);
        assert(gray(e,128));
        assert(sameUv(e,static_cast<unsigned char>(score[i])));
    }
    const auto& cursor=q[base+31u];
    assert(cursor.x==9.f*16.f && cursor.y==5.f*32.f);
    assert(gray(cursor,128));
    assert(sameUv(cursor,static_cast<unsigned char>('_')));

    // r247: handheld D-Pad mode keeps the original row graphics but places
    // the pulse cursor on the selected fixed-width cell rather than after the
    // PC keyboard append length.
    s.recordsDpadNameEditing=true;
    s.recordsNameCursor=5;
    const auto dpad=LegacyHud::compose(s);
    const std::size_t dbase=dpad.size()-48u;
    assert(dpad[dbase+31u].x==12.f*16.f && dpad[dbase+31u].y==5.f*32.f);
    assert(gray(dpad[dbase+31u],128));

    std::cout << "records name visual r244 PASS\n";
}

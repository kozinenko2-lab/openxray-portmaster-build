#include "render/legacy_theme.hpp"
#include <cassert>
#include <cmath>

static bool near(float a,float b,float eps=0.05f){return std::fabs(a-b)<eps;}
int main(){
    const auto& c=kLegacyMainMenuCameraTrace;
    assert(c.cameraCall==0x004133E9u);
    assert(c.steadyAngle2==-512);
    auto r0=c.steadyRect640(0,1.0f);
    assert(near(r0.left,213.333f));
    assert(near(r0.right,426.667f));
    assert(near(r0.top,195.556f));
    assert(near(r0.bottom,231.111f));
    auto rs=c.steadyRect640(0,1.15f);
    assert(rs.left<r0.left && rs.right>r0.right);
    assert(rs.top<r0.top && rs.bottom>r0.bottom);
    auto r4=c.steadyRect640(4,1.0f);
    assert(near((r4.top+r4.bottom)*0.5f,426.667f));

    const auto& g=kLegacyRecordsTextGridTrace;
    assert(g.fontTextureSlot==5);
    assert(g.columns==40 && g.rows==15);
    assert(g.pixelX640(g.headingColumn)==224);
    assert(g.pixelY480(g.headingRow)==32);
    assert(g.pixelX640(g.nameColumn)==112);
    assert(g.pixelX640(g.scoreRightEdgeColumn)==528);
    assert(g.firstScoreRow==3 && g.scoreRows==10);
    assert(g.modeNameBase==0x025B5BCCu && g.modeSourceStride==12);
    assert(g.centeredModeStart(7)==2); // ПРОСТАЯ: 7 CP1251 bytes
    assert(g.centeredModeStart(8)==1); // КЛАССИКА: 8 bytes
    assert(g.alphaEnable==0x0040FAF8u && g.alphaDisable==0x0040FB0Bu);
    const auto& f=kLegacyFnt4GlyphTrace;
    assert(f.columns==16 && f.rows==10);
    assert(f.glyphIndex(static_cast<unsigned char>(' '))==0);
    assert(f.glyphIndex(static_cast<unsigned char>('-'))==13);
    assert(f.glyphIndex(static_cast<unsigned char>('0'))==16);
    assert(f.glyphIndex(static_cast<unsigned char>('A'))==33);
    assert(f.glyphIndex(0xC0u)==96);
    assert(f.glyphIndex(0xDFu)==127);
    const auto uv=f.uvForGlyph(96);
    assert(near(uv[0],0.005859375f));
    assert(near(uv[1],0.009765625f+6.f*0.0984765589f));
    return 0;
}

#include "render/legacy_hud_trace.hpp"
#include "render/legacy_theme.hpp"
#include <cassert>
#include <cstring>
#include <iostream>

int main(){
    const auto& h=kLegacyGameplayHudTrace;
    assert(h.rendererTexture3Call==0x00420641u);
    assert(h.rendererHudCall==0x0042064Cu);
    assert(h.routine==0x004247E0u);
    assert(h.emitNumber==0x0040EC90u);
    assert(h.submitBatch==0x0040ED60u);
    assert(h.layoutInitHighRes==0x004241D0u);
    assert(h.staticQuadBuilder==0x0040EB40u);
    assert(h.glyphAdvance640==16 && h.glyphHeight640==16);
    assert(h.layout640x480[0]==32 && h.layout640x480[1]==4);
    assert(h.layout640x480[2]==576 && h.layout640x480[3]==4);
    assert(h.layout640x480[4]==64 && h.layout640x480[5]==460);
    assert(h.layout640x480[6]==121 && h.layout640x480[7]==460);
    assert(h.layout640x480[8]==544 && h.layout640x480[9]==460);
    assert(kLegacyGameplayHudNumberCalls.size()==5);
    assert(kLegacyGameplayHudNumberCalls[0].valueGlobal==0x0257DA10u);
    assert(kLegacyGameplayHudNumberCalls[0].x640==32 && kLegacyGameplayHudNumberCalls[0].y480==4);
    assert(kLegacyGameplayHudNumberCalls[1].x640==64 && kLegacyGameplayHudNumberCalls[1].y480==460);
    assert(kLegacyGameplayHudNumberCalls[2].x640==576 && kLegacyGameplayHudNumberCalls[2].y480==4);
    assert(kLegacyGameplayHudNumberCalls[3].x640==121 && kLegacyGameplayHudNumberCalls[3].y480==460);
    assert(kLegacyGameplayHudNumberCalls[4].minDigits==6 && kLegacyGameplayHudNumberCalls[4].x640==544);

    assert(kLegacyGameplayHudStaticQuads.size()==5);
    assert(std::strcmp(kLegacyGameplayHudStaticQuads[0].resource,"HEAR")==0);
    assert(kLegacyGameplayHudStaticQuads[0].x640==0 && kLegacyGameplayHudStaticQuads[0].w640==32);
    assert(std::strcmp(kLegacyGameplayHudStaticQuads[1].resource,"CLCK")==0 && kLegacyGameplayHudStaticQuads[1].x640==608);
    assert(std::strcmp(kLegacyGameplayHudStaticQuads[2].resource,"LEVL")==0 && kLegacyGameplayHudStaticQuads[2].y480==460);
    assert(std::strcmp(kLegacyGameplayHudStaticQuads[3].resource,"SCOR")==0 && kLegacyGameplayHudStaticQuads[3].x640==524);
    assert(std::strcmp(kLegacyGameplayHudStaticQuads[4].resource,"PERC")==0 && kLegacyGameplayHudStaticQuads[4].x640==96);

    const auto& t=kLegacyEnvironmentThemeTrace;
    assert(t.tableAddress==0x004418B0u && t.selectRoutine==0x00422FC0u);
    assert(t.uploadRoutine==0x004058F0u && t.themeCount==12);
    assert(std::strcmp(kLegacyEnvironmentThemes[7].slot0,"0212")==0);
    assert(std::strcmp(kLegacyEnvironmentThemes[7].slot1,"VOL0")==0);
    assert(std::strcmp(kLegacyEnvironmentThemes[7].slot2,"0049")==0);
    std::cout<<"hud/theme trace ok\n";
}

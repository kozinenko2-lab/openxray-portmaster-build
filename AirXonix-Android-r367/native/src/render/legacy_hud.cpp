#include "legacy_hud.hpp"
#include <cstdio>
#include "legacy_hud_trace.hpp"
#include "legacy_theme.hpp"
#include "legacy_controls_visual.hpp"
#include "game/legacy_records_transition.hpp"
#include "game/legacy_information_transition.hpp"
#include "game/legacy_controls_trace.hpp"
#include "game/legacy_settings_visual_trace.hpp"
#include "game/legacy_settings_trace.hpp"
#include <algorithm>
#include <string>
#include <cstdint>
#include <cstring>

namespace {
constexpr float kDigitW=16.f;

void sprite(std::vector<LegacyHudSprite>& out,LegacyHudAtlas atlas,
            float x,float y,float w,float h,int sx,int sy,int sw,int sh){
    out.push_back({atlas,x,y,w,h,sx,sy,sw,sh,1.f});
}

void fontText(std::vector<LegacyHudSprite>& out,const std::string& bytes,int column,int row,
              std::uint32_t rgb){
    float x=float(column*16),y=float(row*32);
    const float rr=float((rgb>>16)&0xffu)/255.f;
    const float gg=float((rgb>>8)&0xffu)/255.f;
    const float bb=float(rgb&0xffu)/255.f;
    for(unsigned char ch:bytes){
        const auto uv=kLegacyFnt4GlyphTrace.uvForGlyph(kLegacyFnt4GlyphTrace.glyphIndex(ch));
        LegacyHudSprite q{LegacyHudAtlas::Font5,x,y,16.f,32.f,0,0,0,0,1.f};
        q.r=rr;q.g=gg;q.b=bb;q.u0=uv[0];q.v0=uv[1];q.u1=uv[2];q.v1=uv[3];
        out.push_back(q);x+=16.f;
    }
}


void tracedHudQuad(std::vector<LegacyHudSprite>& out,const LegacyHudStaticQuadTrace& t){
    LegacyHudSprite q{LegacyHudAtlas::Atlas3,float(t.x640),float(t.y480),float(t.w640),float(t.h480),0,0,0,0,1.f};
    q.u0=t.u0; q.v0=t.v0; q.u1=t.u0+t.du; q.v1=t.v0+t.dv;
    out.push_back(q);
}

std::string fixedKeyName(int code){
    const auto a=LegacyControlsVisual::keyName(code);
    std::string out(a.data(),a.size());
    while(!out.empty() && (out.back()==' ' || out.back()=='\0')) out.pop_back();
    return out;
}
}

namespace LegacyHud {
void appendNumber(std::vector<LegacyHudSprite>& out,LegacyHudAtlas atlas,
                  int value,int minDigits,float x,float y,bool rightAlign){
    value=std::max(0,value);
    std::string text=std::to_string(value);
    while(static_cast<int>(text.size())<minDigits)text.insert(text.begin(),'0');
    const float total=kDigitW*static_cast<float>(text.size());
    if(rightAlign)x-=total;
    for(char c:text){
        const int d=c-'0';
        if(atlas==LegacyHudAtlas::Atlas3){
            // r69 DIRECT EXE 0x40E9C0: digit U table spans 0.3125..0.9375
            // in ten equal cells; V is exactly 1/256..21/256. This is a
            // 16x20 sample inside CNT2's 16x24 cells, not the old 16x32 guess.
            LegacyHudSprite q{atlas,x,y,kDigitW,20.f,0,0,0,0,1.f};
            q.u0=(80.f+float(d)*16.f)/256.f; q.u1=q.u0+16.f/256.f;
            q.v0=1.f/256.f; q.v1=21.f/256.f;
            out.push_back(q);
        }else if(atlas==LegacyHudAtlas::Atlas4){
            // r279: CNT3 does have a runtime consumer, but only in the startup
            // level-intro 3-D presentation (0x41D4D0 -> 0x40EE40). It is not
            // the normal 2-D HUD digit strip, so keep Atlas4 disabled here.
            return;
        }else{
            // Atlas #7 has no decimal strip in the recovered table. Keeping
            // this branch empty prevents accidental use of unrelated pixels.
            return;
        }
        x+=kDigitW;
    }
}

std::vector<LegacyHudSprite> compose(const LegacyHudState& s){
    std::vector<LegacyHudSprite> out;
    out.reserve(48);

    if(s.screen==LegacyHudScreen::Gameplay || s.screen==LegacyHudScreen::InterLevel || s.screen==LegacyHudScreen::FinalSequence){
        // r49 DIRECT EXE: exact 640x480 static HUD destinations from
        // 0x424331..0x4244D2 and numeric destinations from 0x4247E0.
        tracedHudQuad(out,kLegacyGameplayHudStaticQuads[0]);                    // HEAR
        const std::size_t livesFirst=out.size();
        appendNumber(out,LegacyHudAtlas::Atlas3,s.lives,(s.lives>=10)?2:1,32,4,false);
        if(s.lives<=1){
            const float pulse=float(s.pulseCounter&0x7f)/127.f;
            for(std::size_t i=livesFirst;i<out.size();++i){out[i].r=1.f;out[i].g=0.5f+0.5f*pulse;out[i].b=0.f;}
        }

        tracedHudQuad(out,kLegacyGameplayHudStaticQuads[1]);                    // CLCK
        const std::size_t timeFirst=out.size();
        appendNumber(out,LegacyHudAtlas::Atlas3,std::max(0,s.timeRemaining>>10),2,576,4,false);
        if((s.timeRemaining>>10)<=10){
            const float pulse=float(s.pulseCounter&0x7f)/127.f;
            for(std::size_t i=timeFirst;i<out.size();++i){out[i].r=1.f;out[i].g=0.5f+0.5f*pulse;out[i].b=0.f;}
        }

        // r48/r49 DIRECT EXE: restore the literal five-field HUD layout.
        // Static quads at 640x480 are LEVL x=0, PERC x=96, SCOR x=524.
        tracedHudQuad(out,kLegacyGameplayHudStaticQuads[2]);                    // LEVL
        appendNumber(out,LegacyHudAtlas::Atlas3,s.levelNumber,2,64,460,false);
        tracedHudQuad(out,kLegacyGameplayHudStaticQuads[4]);                    // PERC
        appendNumber(out,LegacyHudAtlas::Atlas3,std::max(0,s.capturePercent),s.capturePercent>99?3:2,121,460,false);
        tracedHudQuad(out,kLegacyGameplayHudStaticQuads[3]);                    // SCOR
        appendNumber(out,LegacyHudAtlas::Atlas3,s.score,6,544,460,false);

        // r168 direct-EXE correction: finale completion text is the prepared
        // GAME/COMP 3-D pair rendered by 0x41B94C..0x41BA3B. There is no
        // additional flat COMP HUD sprite here; the old duplicate produced the
        // stray "ПРОЙ..." banner during the live finale.
    }else if(s.screen==LegacyHudScreen::MainMenu){
        // r61 DIRECT EXE: steady-state M1 placement is now exact at 640x480.
        // The original smooths scale/brightness toward these targets; native
        // currently renders the confirmed steady targets while preserving the
        // exact source strip and selection-dependent size/brightness.
        constexpr int srcY[5]={0,48,96,144,192};
        const int selected=std::clamp(s.menuSelected,0,4);
        for(int i=0;i<5;++i){
            const bool sel=i==selected;
            (void)sel;
            const float scale=s.menuScale[static_cast<std::size_t>(i)];
            const auto r=kLegacyMainMenuCameraTrace.steadyRect640(i,scale);
            LegacyHudSprite q{LegacyHudAtlas::MenuM1,r.left,r.top,r.right-r.left,r.bottom-r.top,
                              0,srcY[i],256,48,
                              s.menuBrightness[static_cast<std::size_t>(i)]};
            out.push_back(q);
        }
    }else if(s.screen==LegacyHudScreen::ModeSelect){
        // r342 DIRECT EXE 0x4119D6..0x411A68 (registered selector 0x411870).
        // Each row begins with the fixed 11-byte SOUNDINF mode-name field at
        // column 8, followed by the right-aligned level count and "Этапов".
        // The previous r323 interpretation accidentally omitted these names.
        const auto fadeMode=[&](std::uint32_t c){return kLegacyModeSelectPresentationTrace.fadeTextRgb(c,s.modeFadeCounter);};
        fontText(out," \xC2\xDB\xC1\xC5\xD0\xC8\xD2\xC5 \xC8\xC3\xD0\xD3: ",12,5,fadeMode(0x00ff00u));
        const int selected=std::clamp(s.modeSelected,0,4);
        for(int i=0;i<5;++i){
            const std::uint32_t c=fadeMode((i==selected)?0xffffffu:0x008fffu);
            std::string modeName(11,' ');
            const auto& src=s.modeNames[std::size_t(i)];
            const std::size_t n=std::min<std::size_t>(modeName.size(),src.size());
            if(n)modeName.replace(0,n,src.data(),n);
            fontText(out,modeName,8,7+i,c);
            char count[12];std::snprintf(count,sizeof(count),"%d",s.modeLevelCounts[std::size_t(i)]);
            const int len=static_cast<int>(std::strlen(count));
            fontText(out,count,25-len,7+i,c);
            fontText(out,"\xDD\xF2\xE0\xEF\xEE\xE2",26,7+i,c); // Этапов
        }
        // The original recolours the entire pre-existing row 14 through
        // 0x40BFB0. Keep the PortMaster control hint in that row, but feed it
        // through the exact pulse+palette law rather than treating it as part
        // of the original selector text payload.
        fontText(out,"A/ENTER - START   B/ESC - BACK",5,14,
                 fadeMode(kLegacyModeSelectPresentationTrace.footerPackedRgb(s.modeAnglePhase)));
    }else if(s.screen==LegacyHudScreen::Records){
        const std::size_t recordsStart=out.size();
        // r237 DIRECT EXE 0x40F28E..0x40F37B: literal default Records grid.
        // 0x40F2FB writes the title in fixed red; there is no native pulse.
        fontText(out,"\xD0 \xC5 \xCA \xCE \xD0 \xC4 \xDB",14,1,0xff0000u);
        // r327 DIRECT EXE 0x40F97D..0x40F9BE: recolour only the eleven
        // inner heading cells (columns 15..25), preserving the edge cells.
        for(int i=0;i<11;++i){
            const std::uint32_t c=LegacyRecordsTransition::headingWaveRgb(s.recordsHeadingPhase,i);
            const std::size_t at=out.size()-13u+std::size_t(i+1);
            out[at].r=float((c>>16)&0xffu)/255.f;
            out[at].g=float((c>>8)&0xffu)/255.f;
            out[at].b=float(c&0xffu)/255.f;
        }
        // 0x40F310 / 0x40F349: both 26-char separators are pure blue.
        fontText(out,"--------------------------",7,2,0x0000ffu);
        fontText(out,"--------------------------",7,13,0x0000ffu);
        // r325: 0x40F100 always returns an eleven-byte, space-filled field and
        // centres the visible SOUNDINF mode name using floor((11-len)/2).
        // Use the actual resource-backed name parsed by r324 rather than a
        // fixed Russian table so original/custom localized SOUNDINF survives.
        std::string modeLabel(11,' ');
        const std::size_t n=std::min<std::size_t>(11,s.recordsModeName.size());
        const std::size_t left=(11-n)/2;
        if(n)modeLabel.replace(left,n,s.recordsModeName.data(),n);
        fontText(out,modeLabel,15,2,0xffffffu);
        // 0x40F361..0x40F376 repeats the same centred field at row 13.
        fontText(out,modeLabel,15,13,0xffffffu);
        for(int row=3;row<=12;++row){
            const std::size_t entry=static_cast<std::size_t>(row-3);
            // r238 DIRECT EXE 0x40F293..0x40F2F9: 0x40F0A0 has copied the
            // selected 200-byte block to the working name/value buffers. The
            // name is always written as all 16 bytes (no C-string trimming).
            fontText(out,std::string(s.recordsNames[entry].data(),16),7,row,0x00ffffu);
            fontText(out,"..........",23,row,0x00ffffu);
            const std::string value=std::to_string(s.recordsValues[entry]);
            // 0x40F2D0 starts at column 33 and subtracts the decimal length.
            fontText(out,value,33-static_cast<int>(value.size()),row,0xffff00u);
        }
        if(s.recordsNameEntry && s.recordsCandidateRow>=0 && s.recordsCandidateRow<10){
            // r244 DIRECT EXE 0x40F438..0x40F49F: the candidate row is
            // overwritten in white while editing. Score and the one-byte '_'
            // cursor share a grayscale cosine pulse in range 128..254.
            const std::size_t entry=static_cast<std::size_t>(s.recordsCandidateRow);
            const int row=s.recordsCandidateRow+3;
            fontText(out,std::string(s.recordsNames[entry].data(),16),7,row,0xffffffu);
            fontText(out,"..........",23,row,0xffffffu);
            const int g=std::clamp(s.recordsNamePulseByte,0,255);
            const std::uint32_t pulse=(std::uint32_t(g)<<16)|(std::uint32_t(g)<<8)|std::uint32_t(g);
            const std::string value=std::to_string(s.recordsValues[entry]);
            fontText(out,value,33-static_cast<int>(value.size()),row,pulse);
            const int cursor=s.recordsDpadNameEditing
                ? std::clamp(s.recordsNameCursor,0,15)
                : std::clamp(s.recordsTypedNameLength,0,16);
            fontText(out,"_",7+cursor,row,pulse);
            // r326 DIRECT EXE 0x40FA07..0x40FA2F: name-entry Records writes
            // CP1251 "ВВЕДИТЕ ВАШЕ ИМЯ" at col 12,row 14 and then recolours
            // the entire row with the same cosine grayscale pulse.
            fontText(out,"\xC2\xC2\xC5\xC4\xC8\xD2\xC5 \xC2\xC0\xD8\xC5 \xC8\xCC\xDF",12,14,pulse);
        }
        // r328: 0x40BF40 applies one palette level to the complete 40x15 grid
        // after all per-cell recolours, including the heading and name prompt.
        for(std::size_t i=recordsStart;i<out.size();++i){
            auto scale=[&](float c){
                const int b=std::clamp(int(c*255.f+0.5f),0,255);
                return float(LegacyRecordsTransition::fadeTextByte(b,s.recordsFadeCounter))/255.f;
            };
            out[i].r=scale(out[i].r); out[i].g=scale(out[i].g); out[i].b=scale(out[i].b);
        }
    }else if(s.screen==LegacyHudScreen::Information){
        const std::size_t informationStart=out.size();
        // r165 DIRECT EXE 0x410CF0 dispatches three sequential fnt4 pages:
        // 0x4101B0 rules, 0x4104A0 objects/bonuses, 0x414170 extra enemies.
        // All CP1251 text, rows, columns and packed colours below are literal.
        const int page=std::clamp(s.informationPage,0,2);
        if(page==0){
            fontText(out,"\xCF\xD0\xC0\xC2\xC8\xCB\xC0 \xC8\xC3\xD0\xDB:",14,0,0x00ff00u);
            static const char* lines[]={
                "   \xC2\xFB \xF3\xEF\xF0\xE0\xE2\xEB\xFF\xE5\xF2\xE5 \xF3\xF1\xF2\xF0\xEE\xE9\xF1\xF2\xE2\xEE\xEC, \xF1\xEF\xEE\xF1\xEE\xE1\xED\xFB\xEC ",
                " \xEF\xE5\xF0\xE5\xEC\xE5\xF9\xE0\xF2\xFC\xF1\xFF  \xED\xE0\xE4 \xE8\xE3\xF0\xEE\xE2\xFB\xEC \xEF\xEE\xEB\xE5\xEC.  \xC2\xE0\xF8\xE0 ",
                " \xE7\xE0\xE4\xE0\xF7\xE0 - \xE7\xE0\xF5\xE2\xE0\xF2\xE8\xF2\xFC \xE1\xEE\xEB\xFC\xF8\xF3\xFE \xF7\xE0\xF1\xF2\xFC \xEF\xEE\xEB\xFF. ",
                " \xD7\xF2\xEE\xE1\xFB \xF1\xE4\xE5\xEB\xE0\xF2\xFC \xFD\xF2\xEE,  \xED\xE5\xEE\xE1\xF5\xEE\xE4\xE8\xEC\xEE  \xEB\xE5\xF2\xE0\xF2\xFC ",
                " \xED\xE0\xE4 \xED\xE5\xE7\xE0\xEF\xEE\xEB\xED\xE5\xED\xED\xFB\xEC\xE8 \xF3\xF7\xE0\xF1\xF2\xEA\xE0\xEC\xE8 \xE8 \xEE\xF2\xF0\xE5\xE7\xE0\xF2\xFC",
                " \xF7\xE0\xF1\xF2\xE8 \xEF\xEE\xEB\xFF, \xF1\xE2\xEE\xE1\xEE\xE4\xED\xFB\xE5  \xEE\xF2 \xF8\xE0\xF0\xEE\xE2.  \xD8\xE0\xF0\xFB ",
                " \xED\xE5 \xE4\xEE\xEB\xE6\xED\xFB \xEF\xE5\xF0\xE5\xF1\xE5\xEA\xE0\xF2\xFC  \xE2\xE0\xF8\xF3 \xF2\xF0\xE0\xE5\xEA\xF2\xEE\xF0\xE8\xFE! ",
                " \xC8\xE7\xE1\xE5\xE3\xE0\xE9\xF2\xE5 \xEA\xEE\xED\xF2\xE0\xEA\xF2\xE0 \xF1  \xEC\xE8\xED\xE0\xEC\xE8,  \xEA\xEE\xF2\xEE\xF0\xFB\xE5 ",
                " \xE4\xE2\xE8\xE6\xF3\xF2\xF1\xFF  \xEF\xEE  \xE7\xE0\xEF\xEE\xEB\xED\xE5\xED\xED\xEE\xE9  \xF7\xE0\xF1\xF2\xE8 \xEF\xEE\xEB\xFF. ",
                " \xC2\xF0\xE5\xEC\xFF  \xED\xE0  \xEF\xF0\xEE\xF5\xEE\xE6\xE4\xE5\xED\xE8\xE5  \xEA\xE0\xE6\xE4\xEE\xE3\xEE  \xFD\xF2\xE0\xEF\xE0 ",
                " \xEE\xE3\xF0\xE0\xED\xE8\xF7\xE5\xED\xEE - 60 \xF1\xE5\xEA. \xD1\xEE\xE1\xE8\xF0\xE0\xE9\xF2\xE5  \xE1\xEE\xED\xF3\xF1\xFB ",
                " \xE4\xEB\xFF \xEF\xEE\xEF\xEE\xEB\xED\xE5\xED\xE8\xFF  \xE6\xE8\xE7\xED\xE5\xE9, \xE2\xF0\xE5\xEC\xE5\xED\xE8, \xF1\xF7\xE5\xF2\xE0 ",
                " \xE8 \xE7\xE0\xEC\xE5\xE4\xEB\xE5\xED\xE8\xFF \xE2\xF0\xE0\xE6\xE5\xF1\xEA\xE8\xF5 \xF8\xE0\xF0\xEE\xE2 \xE8 \xEC\xE8\xED.    "};
            for(int i=0;i<13;++i)fontText(out,lines[i],0,i+1,0x00ffffu);
        }else if(page==1){
            fontText(out,"         \xCE\xC1\xDA\xC5\xCA\xD2\xDB \xC8 \xC1\xCE\xCD\xD3\xD1\xDB \xC8\xC3\xD0\xDB:         ",0,0,0x00ff00u);
            fontText(out,"     \xC2\xE0\xF8\xE5 \xF3\xF1\xF2\xF0\xEE\xE9\xF1\xF2\xE2\xEE:                   ",0,2,0x00ffffu);
            fontText(out,"      - \xCB\xE5\xE3\xEA\xE8\xE9 \xF8\xE0\xF0                      ",0,4,0x00ffffu);
            fontText(out,"      - \xD2\xFF\xE6\xE5\xEB\xFB\xE9 \xF8\xE0\xF0        - \xCC\xE8\xED\xE0       ",0,6,0x00ffffu);
            fontText(out,"      - \xD1\xFE\xF0\xEF\xF0\xE8\xE7            - \xD2\xF3\xF0\xE1\xEE \xE1\xEE\xED\xF3\xF1",0,8,0x00ffffu);
            fontText(out,"      - \xC1\xEE\xED\xF3\xF1 \xE6\xE8\xE7\xED\xE8        - \xC1\xEE\xED\xF3\xF1 \xF1\xF7\xE5\xF2\xE0",0,10,0x00ffffu);
            fontText(out,"      - \xC1\xEE\xED\xF3\xF1 \xE2\xF0\xE5\xEC\xE5\xED\xE8      - \xC7\xE0\xEC\xE5\xE4\xEB\xE8\xF2\xE5\xEB\xFC",0,12,0x00ffffu);
        }else{
            fontText(out,"      \xC4\xCE\xCF\xCE\xCB\xCD\xC8\xD2\xC5\xCB\xDC\xCD\xDB\xC5 \xC2\xD0\xC0\xC3\xC8:             ",0,2,0x00ff00u);
            fontText(out,"       \xCF\xEE\xFF\xE2\xEB\xFF\xFE\xF2\xF1\xFF \xED\xE0\xF7\xE8\xED\xE0\xFF               ",0,4,0x00ffffu);
            fontText(out,"        \xF1 \xEC\xE8\xF1\xF1\xE8\xE8 '\xCC\xCE\xC4\xC5\xD0\xCD'               ",0,5,0x00ffffu);
            fontText(out,"  \xCB\xE5\xF2\xE0\xFE\xF9\xE0\xFF \xEC\xE8\xED\xE0:                        ",0,9,0x00ffffu);
            fontText(out,"                         \xC3\xF0\xE5\xE9\xE4\xE5\xF0:       ",0,10,0x00ffffu);
        }
        { const std::uint32_t c=LegacyInformationTransition::promptRgb(s.informationPromptPhase);
          fontText(out,"\xCD\xE0\xE6\xEC\xE8\xF2\xE5 \xEB\xFE\xE1\xF3\xFE \xEA\xED\xEE\xEF\xEA\xF3",10,14,c); }
        // r329 DIRECT EXE 0x41046A/0x4106D2/0x4143C9 -> 0x40BF40:
        // apply level=(fadeCounter>>6) to the complete Information text grid.
        for(std::size_t i=informationStart;i<out.size();++i){
            auto scale=[&](float c){
                const int b=std::clamp(int(c*255.f+0.5f),0,255);
                return float(LegacyInformationTransition::fadeTextByte(b,s.informationFadeCounter))/255.f;
            };
            out[i].r=scale(out[i].r); out[i].g=scale(out[i].g); out[i].b=scale(out[i].b);
        }
    }else if(s.screen==LegacyHudScreen::Settings){
        // r153: 0x413D02..0x4140C1. M2 labels are atlas-4 resources in the
        // exact constructor order M250/M260/M240/M210/M220/M230. The original
        // submits them in the same menu camera family as M1; steady-state
        // projection therefore uses the recovered 640x480 focal/depth mapping.
        constexpr int srcY[6]={0,42,84,126,168,210};
        constexpr float focal=320.f,halfW=320.f,halfH=240.f,depth=.018f;
        const float cameraZ=LegacySettingsTrace::cameraZ(s.settingsSelectorOffset);
        constexpr float baseHalfW=.006f,baseHalfH=.000984375f; // 256x42 aspect
        for(int i=0;i<6;++i){
            const float sc=s.settingsScale[std::size_t(i)];
            // r342 correction: 0x413EE4 submits every M2 label at X=-.006.
            // The old 2-D compatibility projection silently used X=0 and
            // therefore centred the Settings rows instead of left-aligning them.
            const float cx=halfW+kLegacySettingsVisualTrace.labelX*focal/depth;
            const float rowZ=kLegacySettingsVisualTrace.rowZ(i);
            const float cy=halfH+(cameraZ-rowZ)*focal/depth;
            const float px=baseHalfW*sc*focal/depth;
            const float py=baseHalfH*sc*focal/depth;
            out.push_back({LegacyHudAtlas::MenuM2,cx-px,cy-py,px*2.f,py*2.f,
                           0,srcY[i],256,42,s.settingsBrightness[std::size_t(i)]*s.settingsFadeScale});
        }
        // 0x413F4E switches to texture slot 3 for speech and slider controls.
        // on++/off+ are exact 64x38 resources at atlas3 x=128/192, y=0.
        const int speechX=s.settingsSpeech?128:192;
        const float speechCy=halfH+(cameraZ-kLegacySettingsVisualTrace.speechZ)*focal/depth;
        const std::size_t speechIndex=out.size();
        sprite(out,LegacyHudAtlas::Atlas3,385.f,speechCy-19.f,64.f,38.f,speechX,0,64,38);
        out[speechIndex].brightness=s.settingsBrightness[3]*s.settingsFadeScale;

        // r154: the three slider tracks/knobs are not HUD quads. They are
        // exact 3D 0x402A50 meshes (0x257F584/0x257F580) rendered by Renderer
        // with texture slots 4/3 respectively. Keep only the speech mesh in
        // this 2D compatibility command list.
        // Native debug rows requested for hardware tuning. Keep them visually
        // neutral instead of labelling the menu as a TEST build.
        char testLine[48];
        std::snprintf(testLine,sizeof(testLine),"LIVES: %02d",s.testInitialLives);
        fontText(out,testLine,24,0,s.settingsSelected==6?0x00ff00u:0xafafafu);
        std::snprintf(testLine,sizeof(testLine),"TIME : %03d",s.testInitialTimeSeconds);
        fontText(out,testLine,24,1,s.settingsSelected==7?0x00ff00u:0xafafafu);
    }else if(s.screen==LegacyHudScreen::Controls){
        // r155 DIRECT EXE: 0x410E3A..0x411254 writes a 40x15 text grid and
        // renders it with fnt4 in logical texture slot 5. Byte strings below
        // are the literal CP1251 text records at 0x441558..0x4413D4.
        fontText(out,"      \xCD\xC0\xD1\xD2\xD0\xCE\xC9\xCA\xC0 \xCA\xCD\xCE\xCF\xCE\xCA \xD3\xCF\xD0\xC0\xC2\xCB\xC5\xCD\xC8\xDF       ",0,0,0x00ffffu);
        fontText(out,"      ( \xEA\xEB\xE0\xE2\xE8\xE0\xF2\xF3\xF0\xE0 \xE8\xEB\xE8 \xE4\xE6\xEE\xE9\xF1\xF2\xE8\xEA )       ",0,1,0x00ffffu);
        fontText(out,"      ---------------------------       ",0,2,0x00ffffu);
        fontText(out," \xC4\xC5\xC9\xD1\xD2\xC2\xC8\xDF:          \xCA\xCD\xCE\xCF\xCA\xC8:             ",0,4,0xafafafu);
        fontText(out,"\xCF\xEE\xF1\xF2\xEE\xFF\xED\xED\xFB\xE5:",29,6,0x7f7f7fu);
        fontText(out,"\xD1\xF2\xF0. \xC2\xC2\xC5\xD0\xD5 ",29,7,0x7f7f7fu);
        fontText(out,"\xD1\xF2\xF0. \xC2\xCD\xC8\xC7  ",29,8,0x7f7f7fu);
        fontText(out,"\xD1\xF2\xF0. \xC2\xCB\xC5\xC2\xCE ",29,9,0x7f7f7fu);
        fontText(out,"\xD1\xF2\xF0. \xC2\xCF\xD0\xC0\xC2\xCE",29,10,0x7f7f7fu);
        fontText(out,"\xCD\xE0\xE6\xEC\xE8\xF2\xE5 \xEA\xED\xEE\xEF\xEA\xF3, \xF1\xEE\xEE\xF2\xE2\xE5\xF2\xF1\xF2\xE2\xF3\xFE\xF9\xF3\xFE \xE4\xE5\xE9\xF1\xF2\xE2\xE8\xFE",0,12,s.controlsPromptColor);
        fontText(out,"       \xCD\xE0\xE6\xEC\xE8\xF2\xE5 ESCAPE \xE4\xEB\xFF \xEE\xF2\xEC\xE5\xED\xFB.       ",0,14,0xff0000u);
        const char* actions[4]={"\xC4\xE2\xE8\xE6. \xC2\xCF\xD0\xC0\xC2\xCE  ","\xC4\xE2\xE8\xE6. \xC2\xCB\xC5\xC2\xCE   ","\xC4\xE2\xE8\xE6. \xCD\xC0\xC7\xC0\xC4   ","\xC4\xE2\xE8\xE6. \xC2\xCF\xC5\xD0\xC5\xC4  "};
        for(int i=0;i<4;++i){
            const bool current=i==s.controlsAssigned && s.controlsAssigned<4;
            fontText(out,actions[i],0,7+i,current?s.controlsCurrentRowColor:0xcfcf40u);
            fontText(out,fixedKeyName(s.controlsBindings[std::size_t(i)]),17,7+i,current?s.controlsCurrentRowColor:0x9f9fffu);
        }
        if(s.controlsAwaitingConfirm)
            fontText(out,"    \xCD\xE0\xE6\xEC\xE8\xF2\xE5 ENTER \xE4\xEB\xFF \xEF\xEE\xE4\xF2\xE2\xE5\xF0\xE6\xE4\xE5\xED\xE8\xFF.    ",0,12,s.controlsPromptColor);
        // r332 DIRECT EXE 0x41124C..0x411254 -> 0x40BF40: the complete fnt4
        // grid uses the 32-level palette law with level=(counter>>6). The
        // nearby 0x40C190(counter>>3) is not the text-grid fade itself.
        for(auto& q:out){
            auto scale=[&](float c){
                const int b=std::clamp(int(c*255.f+0.5f),0,255);
                return float(kLegacyControlsTrace.fadeTextByte(b,s.controlsFadeCounter))/255.f;
            };
            q.r=scale(q.r); q.g=scale(q.g); q.b=scale(q.b);
        }
    }else if(s.screen==LegacyHudScreen::Complete){
        // r141: no standalone Complete HUD exists in the original flow. COMP
        // is cinematic slot 0 rendered from 0x41B2A0 before that routine RETs.
    }else if(s.screen==LegacyHudScreen::GameOver){
        // r132: GOVE is cinematic prepared slot 3 rendered in world/presentation
        // space, not a 2D HUD blit.
    }else if(s.screen==LegacyHudScreen::Abort){
        // r133: ABOR is cinematic prepared slot 4, not a 2D HUD blit.
    }
    return out;
}
}

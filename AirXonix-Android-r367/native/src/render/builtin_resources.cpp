#include "builtin_resources.hpp"
#include "legacy_atlas_runtime.hpp"
#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>

namespace BuiltinResources {
namespace {
std::uint32_t fourccHash(const std::string& name){
    std::uint32_t h=2166136261u;for(unsigned char c:name){h^=c;h*=16777619u;}return h;
}
std::pair<int,int> dimensions(const std::string& n){
    // r152: preserve original BMPPACK source extents. The legacy atlas copier
    // copies native source rectangles verbatim, so convenience dimensions can
    // overwrite neighbours or change normalized UV sampling.
    if(n=="BALL"||n=="XONI"||n=="SPEE"||n=="MONY"||n=="HEAR"||n=="CLCK") return {32,32};
    if(n=="CNT2") return {192,24};
    if(n=="CNT3") return {256,36};
    if(n=="LEVL") return {64,24};
    if(n=="SCOR"||n=="PERC") return {24,24};
    if(n=="PAUS") return {64,24};
    if(n=="XON1") return {256,32};
    if(n=="VZRV") return {16,16};
    if(n=="VZR1") return {64,64};
    if(n=="SHAD") return {16,16};
    if(n=="GOVE"||n=="COMP"||n=="cmp2") return {256,48};
    if(n=="ABOR") return {256,32};
    if(n=="GAME"||n=="gam2") return {128,48};
    if(n=="RAM3") return {128,92};
    if(n=="IN2$"||n=="IN2T"||n=="IN2L"||n=="IN2S"||n=="IN2A") return {128,48};
    if(n=="TOU2") return {256,48};
    if(n=="LEV2") return {256,64};
    if(n=="1111") return {128,128};
    if(n=="LOGO"||n=="fnt4") return {256,256};
    if(n=="on++"||n=="off+") return {64,38};
    if(n=="TEMP") return {256,4};
    if(n.size()==4&&n[0]=='M'&&n[1]=='1') return {256,48};
    if(n.size()==4&&n[0]=='M'&&n[1]=='2') return {256,42};
    return {64,64};
}
void pixel(LegacyAtlasImage& out,int x,int y,std::uint8_t r,std::uint8_t g,std::uint8_t b,std::uint8_t a=255){
    if(x<0||y<0||x>=out.width||y>=out.height)return;
    auto* p=out.rgba.data()+(std::size_t(y)*std::size_t(out.width)+std::size_t(x))*4u;
    p[0]=r;p[1]=g;p[2]=b;p[3]=a;
}
void glyph(LegacyAtlasImage& out,char c,int ox,int oy,int scale,std::uint8_t r,std::uint8_t g,std::uint8_t b){
    const std::uint32_t bits=fourccHash(std::string(1,c));
    for(int y=0;y<7;++y)for(int x=0;x<5;++x){const bool edge=(x==0||x==4||y==0||y==6);const bool on=edge?((bits>>((x+y*3)&31))&1u):((bits>>((x*5+y*7)&31))&1u);if(!on)continue;for(int yy=0;yy<scale;++yy)for(int xx=0;xx<scale;++xx)pixel(out,ox+x*scale+xx,oy+y*scale+yy,r,g,b,255);}
}
}

bool buildTexture(const std::string& name,LegacyAtlasImage& out){
    const auto [w,h]=dimensions(name);out.width=w;out.height=h;out.rgba.assign(std::size_t(w)*std::size_t(h)*4u,0u);const std::uint32_t seed=fourccHash(name);
    if(name=="1111"){
        // Clean-room replacement for the legacy environment-map texture. The
        // original BMPPACK path colour-keys black to alpha=0; preserve that
        // rendering contract without embedding the commercial bitmap.
        const float cx=float(w-1)*0.36f,cy=float(h-1)*0.32f;
        for(int y=0;y<h;++y)for(int x=0;x<w;++x){
            const float dx=(float(x)-cx)/(float(w)*0.43f),dy=(float(y)-cy)/(float(h)*0.43f);
            const float q=dx*dx+dy*dy;
            if(q>=1.f)continue;
            const float fall=(1.f-q);const float hot=fall*fall;
            const auto a=std::uint8_t(std::clamp(220.f*hot,0.f,220.f));
            const auto r=std::uint8_t(std::clamp(125.f+125.f*hot,0.f,255.f));
            const auto g=std::uint8_t(std::clamp(150.f+105.f*hot,0.f,255.f));
            pixel(out,x,y,r,g,255,a);
        }
        return true;
    }
    const bool opaqueEnv=(w==64&&h==64&&name!="SHAD"&&name!="VZR1")||(!name.empty()&&(name[0]=='0'||name.rfind("VOL",0)==0||name.rfind("SK",0)==0||name.rfind("TST",0)==0));
    for(int y=0;y<h;++y)for(int x=0;x<w;++x)if(opaqueEnv){const int checker=((x/8)^(y/8))&1;const int wave=(x*3+y*5+int(seed&31u))&31;pixel(out,x,y,std::uint8_t(28+((seed>>16)&63u)+checker*18+wave/3),std::uint8_t(36+((seed>>8)&79u)+checker*12+wave/2),std::uint8_t(42+(seed&79u)+checker*20+wave/3),255);}
    if(name=="SHAD"){const float cx=(w-1)*.5f,cy=(h-1)*.5f;for(int y=0;y<h;++y)for(int x=0;x<w;++x){const float dx=(x-cx)/(w*.5f),dy=(y-cy)/(h*.5f),q=dx*dx+dy*dy;if(q<1.f)pixel(out,x,y,0,0,0,std::uint8_t(90.f*(1.f-q)));}return true;}
    if(!opaqueEnv){const std::uint8_t r=std::uint8_t(96+((seed>>16)&127u)),g=std::uint8_t(96+((seed>>8)&127u)),b=std::uint8_t(96+(seed&127u));for(int y=2;y<h-2;++y)for(int x=2;x<w-2;++x){const bool border=(x<4||y<4||x>=w-4||y>=h-4);pixel(out,x,y,border?std::uint8_t(std::min(255,int(r)+35)):r,border?std::uint8_t(std::min(255,int(g)+35)):g,border?std::uint8_t(std::min(255,int(b)+35)):b,border?235:190);}const int scale=h>=42?3:2;int ox=6;const int oy=std::max(3,(h-7*scale)/2);for(char c:name){if(ox+5*scale>=w-3)break;glyph(out,c,ox,oy,scale,245,245,245);ox+=6*scale;}}
    return true;
}
}

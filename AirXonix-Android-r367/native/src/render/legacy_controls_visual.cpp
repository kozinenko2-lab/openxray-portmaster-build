#include "legacy_controls_visual.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace LegacyControlsVisual {
namespace {
std::array<char,8> fixed(const char* s){
    // The original table stores exactly eight inline bytes, not C strings.
    std::array<char,8> out{{' ',' ',' ',' ',' ',' ',' ',' '}};
    const std::size_t n=std::min<std::size_t>(8,std::strlen(s));
    std::memcpy(out.data(),s,n);
    return out;
}
}
std::array<char,8> keyName(int c){
    if((c>='0'&&c<='9')||(c>='A'&&c<='Z')){char s[2]={char(c),0};return fixed(s);}
    if(c>=0x70&&c<=0x78){char s[16]{};std::snprintf(s,sizeof(s),"F%d",c-0x6f);return fixed(s);}
    switch(c){
        case 0x09:return fixed("TAB"); case 0x10:return fixed("SHIFT");
        case 0x21:return fixed("PgUp"); case 0x22:return fixed("PgDown");
        case 0x23:return fixed("End"); case 0x24:return fixed("Home");
        case 0x6A:return fixed("MUL"); case 0x6B:return fixed("ADD");
        case 0x6D:return fixed("SUB"); case 0x6E:return fixed("DECIMAL");
        case 0x6F:return fixed("DIV"); case 0x2D:return fixed("INS"); case 0x2E:return fixed("DEL");
        default:break;
    }
    if(c>=0x60&&c<=0x69){char s[16]{};std::snprintf(s,sizeof(s),"Num%d",c-0x60);return fixed(s);}
    if(c>=0x100&&c<=0x11B){
        const bool j2=c>=0x10E; const int d=c-(j2?0x10E:0x100);
        const char j=j2?'2':'1'; char s[16]{};
        if((!j2&&d<=3)||(j2&&d>=10)){
            const int dir=j2?d-10:d;
            const char* n=dir==0?"Right":dir==1?"Left":dir==2?"Down":"Up";
            std::snprintf(s,sizeof(s),"J%c-%s",j,n);
        }else{
            const int b=j2?d+1:d-3;
            std::snprintf(s,sizeof(s),"J%c-but%d",j,b);
        }
        return fixed(s);
    }
    return fixed("?");
}
} // namespace LegacyControlsVisual

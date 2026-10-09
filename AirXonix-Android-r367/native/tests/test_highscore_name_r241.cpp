#include "core/legacy_highscore.hpp"

#include <array>
#include <cassert>
#include <iostream>

using namespace airxonix;

static std::array<char,16> dots(){
    std::array<char,16> n{}; n.fill('.'); return n;
}

int main(){
    {
        auto n=dots(); int len=0;
        auto r=applyLegacyHighScoreNameInput(n,len,'A');
        assert(r.action==LegacyHighScoreNameAction::Edited && r.sfx==0x15);
        assert(len==1 && n[0]=='A' && n[1]=='.');
        r=applyLegacyHighScoreNameInput(n,len,'z');
        assert(len==2 && n[1]=='z');
        r=applyLegacyHighScoreNameInput(n,len,0xC0);
        assert(len==3 && static_cast<unsigned char>(n[2])==0xC0);
        r=applyLegacyHighScoreNameInput(n,len,' ');
        assert(len==4 && n[3]==' ');
    }
    {
        auto n=dots(); int len=0;
        auto r=applyLegacyHighScoreNameInput(n,len,0xA8); // CP1251 Ё is outside C0..FF
        assert(r.action==LegacyHighScoreNameAction::Edited && r.sfx==0x15);
        assert(len==1 && n[0]=='~');
        r=applyLegacyHighScoreNameInput(n,len,0x08);
        assert(r.action==LegacyHighScoreNameAction::Edited && len==0 && n[0]=='.');
        r=applyLegacyHighScoreNameInput(n,len,0x08);
        assert(r.action==LegacyHighScoreNameAction::None && r.sfx==-1 && len==0);
    }
    {
        auto n=dots(); int len=0;
        auto r=applyLegacyHighScoreNameInput(n,len,0x1B);
        assert(r.action==LegacyHighScoreNameAction::Cancelled && r.sfx==0x16);
        n=dots(); len=0;
        applyLegacyHighScoreNameInput(n,len,'A');
        r=applyLegacyHighScoreNameInput(n,len,0x1B);
        assert(r.action==LegacyHighScoreNameAction::None && len==1);
    }
    {
        auto n=dots(); int len=0;
        // Enter on empty/dot/space-only name resets the typed length and stays.
        applyLegacyHighScoreNameInput(n,len,' ');
        applyLegacyHighScoreNameInput(n,len,'.');
        auto r=applyLegacyHighScoreNameInput(n,len,0x0D);
        assert(r.action==LegacyHighScoreNameAction::None && r.sfx==-1 && len==0);
        for(char c:n) assert(c=='.');
    }
    {
        auto n=dots(); int len=0;
        applyLegacyHighScoreNameInput(n,len,'A');
        applyLegacyHighScoreNameInput(n,len,'B');
        applyLegacyHighScoreNameInput(n,len,' ');
        applyLegacyHighScoreNameInput(n,len,'.');
        auto r=applyLegacyHighScoreNameInput(n,len,0x0D);
        assert(r.action==LegacyHighScoreNameAction::Confirmed && r.sfx==0x16);
        assert(n[0]=='A' && n[1]=='B');
        for(std::size_t i=2;i<n.size();++i) assert(n[i]=='.');
    }
    {
        auto n=dots(); int len=0;
        for(int i=0;i<16;++i){
            auto r=applyLegacyHighScoreNameInput(n,len,'A'+(i%26));
            assert(r.action==LegacyHighScoreNameAction::Edited);
        }
        assert(len==16);
        const auto before=n;
        auto r=applyLegacyHighScoreNameInput(n,len,'Q');
        assert(r.action==LegacyHighScoreNameAction::None && len==16 && n==before);
        r=applyLegacyHighScoreNameInput(n,len,0x1234);
        assert(r.action==LegacyHighScoreNameAction::None && len==16 && n==before);
    }
    {
        auto n=dots(); int len=0;
        auto r=applyLegacyHighScoreNameInput(n,len,0);
        assert(r.action==LegacyHighScoreNameAction::None && len==0);
        r=applyLegacyHighScoreNameInput(n,len,0x1234);
        assert(r.action==LegacyHighScoreNameAction::Edited && n[0]=='~' && len==1);
    }

    std::cout << "highscore name r241 PASS\n";
    return 0;
}

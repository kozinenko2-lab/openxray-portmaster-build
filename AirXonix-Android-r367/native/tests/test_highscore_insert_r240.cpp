#include "core/legacy_highscore.hpp"
#include <cassert>
#include <iostream>

static airxonix::LegacyHighScoreBlock ranked(){
    airxonix::LegacyHighScoreBlock b{};
    for(int i=0;i<10;++i){
        b.names[std::size_t(i)].fill(char('A'+i));
        b.values[std::size_t(i)]=std::uint32_t(1000-i*100);
    }
    return b;
}

int main(){
    { // beats all: row 0, old row 0 shifts to 1
        auto b=ranked();
        assert(airxonix::insertLegacyHighScoreCandidate(b,1100u)==0);
        assert(b.values[0]==1100u && b.values[1]==1000u);
        for(char c:b.names[0])assert(c=='.');
        for(char c:b.names[1])assert(c=='A');
    }
    { // between 1000 and 900
        auto b=ranked();
        assert(airxonix::insertLegacyHighScoreCandidate(b,950u)==1);
        assert(b.values[0]==1000u && b.values[1]==950u && b.values[2]==900u);
        for(char c:b.names[2])assert(c=='B');
    }
    { // equality is stable: new 900 goes after existing 900
        auto b=ranked();
        assert(airxonix::insertLegacyHighScoreCandidate(b,900u)==2);
        assert(b.values[1]==900u && b.values[2]==900u && b.values[3]==800u);
        for(char c:b.names[1])assert(c=='B');
        for(char c:b.names[2])assert(c=='.');
    }
    { // strictly greater than tenth enters at row 9
        auto b=ranked();
        assert(airxonix::insertLegacyHighScoreCandidate(b,101u)==9);
        assert(b.values[8]==200u && b.values[9]==101u);
    }
    { // equal/below tenth is rejected without mutation
        for(auto score:{100u,99u,0u}){
            auto b=ranked();const auto before=b;
            assert(airxonix::insertLegacyHighScoreCandidate(b,score)==-1);
            assert(b.names==before.names && b.values==before.values);
        }
    }
    std::cout << "highscore insert r240 PASS\n";
}

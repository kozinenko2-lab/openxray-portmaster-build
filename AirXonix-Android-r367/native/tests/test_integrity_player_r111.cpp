#include "game/legacy_level_integrity_trace.hpp"
#include "game/player.hpp"
#include <array>
#include <cassert>
int main(){
    static_assert(LegacyPlayerGridTrace::initialX==32 && LegacyPlayerGridTrace::initialY==-2);
    Player p;p.reset();assert(p.worldX()>0.49f&&p.worldX()<0.51f);assert(p.worldZ()<0.4f);
    std::array<std::uint8_t,6> b{{1,2,3,4,5,6}};
    std::uint8_t a=0x6b;for(std::size_t i=0;i<b.size();++i)a=std::uint8_t((i&1)?a+b[i]:a-b[i]);a&=0x3f;
    std::uint8_t c=0x45;for(std::size_t i=0;i<b.size();++i)c=std::uint8_t(c+std::uint8_t(std::uint8_t(i*2)-b[i]));c&=0x3f;
    assert(LegacyLevelIntegrityTrace::checksumA(b.data(),b.size())==a);
    assert(LegacyLevelIntegrityTrace::checksumB(b.data(),b.size())==c);
}

#include "audio/legacy_audio_trace.hpp"
#include "render/legacy_theme.hpp"
#include <array>
#include <cassert>
#include <cstdint>

static std::size_t refMusic(std::array<std::uint32_t,10>& u,int r){
    auto m=*std::min_element(u.begin(),u.end()); int want=(static_cast<unsigned>(r)&0x15u)+1,seen=0; std::size_t i=0;
    for(;;){i=(i+1)%10;if(u[i]==m&&++seen>=want){++u[i];return i;}}
}
static std::size_t refEnv(std::array<std::uint32_t,12>& u,int r){
    auto m=*std::min_element(u.begin(),u.end()); int want=(static_cast<unsigned>(r)&7u)+1,seen=0; std::size_t i=0;
    for(;;){i=(i+1)%12;if(u[i]==m&&++seen>=want){++u[i];return i;}}
}
int main(){
    std::array<std::uint32_t,10> a{},b{}; airxonix::LegacyMusicSelectorTrace::reset(a); airxonix::LegacyMusicSelectorTrace::reset(b);
    assert(a[0]==1); for(int r: {0,1,2,7,8,10,21,22,31,0x1234}) assert(airxonix::LegacyMusicSelectorTrace::select(a,r)==refMusic(b,r));
    std::array<std::uint32_t,12> c{},d{}; for(int r: {0,1,7,8,11,12,21,0x1234}) assert(LegacyEnvironmentThemeSelectorTrace::select(c,r)==refEnv(d,r));
    return 0;
}

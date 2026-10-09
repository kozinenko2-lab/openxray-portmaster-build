#include "game/game.hpp"
#include "game/legacy_settings_trace.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

struct GameTestProbe {
    static SettingsState& settings(Game& g){return g.settings_;}
    static std::int32_t& reserved(Game& g){return g.legacySelectedMode_;}
};

static std::vector<unsigned char> readAll(const std::filesystem::path& p){
    std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};
}
int main(){
    const auto p=std::filesystem::temp_directory_path()/"airxonix-r301-gameinf.bin";
    std::error_code ec;std::filesystem::remove(p,ec);
    Game g;
    assert(g.initializeLegacySettings(p));
    auto b=readAll(p); assert(b.size()==LegacySettingsTrace::fileSize);
    const float* fp=reinterpret_cast<const float*>(b.data());
    assert(fp[0]==800.f && fp[1]==1000.f && fp[2]==800.f);
    const std::int32_t* ip=reinterpret_cast<const std::int32_t*>(b.data());
    assert(ip[3]==0 && ip[4]==0x41 && ip[5]==0x5a && ip[6]==0x58 && ip[7]==0x43 && ip[8]==1);

    auto& s=GameTestProbe::settings(g);s.speed=925.f;s.sfx=375.f;s.music=625.f;s.bindings={{1,2,3,4}};s.speech=false;GameTestProbe::reserved(g)=0x12345678;
    assert(g.saveLegacySettingsNow());
    assert(std::filesystem::file_size(p)==LegacySettingsTrace::fileSize);
    Game h;assert(h.initializeLegacySettings(p));
    const auto& hs=h.settings();
    assert(hs.speed==925.f && hs.sfx==375.f && hs.music==625.f);
    assert((hs.bindings==std::array<int,4>{{1,2,3,4}})); assert(!hs.speech);
    assert(GameTestProbe::reserved(h)==0x12345678);

    // A short file follows 0x4230C0 -> 0x423040 and is rewritten to defaults.
    {std::ofstream f(p,std::ios::binary|std::ios::trunc);char x=0;f.write(&x,1);}
    Game d;assert(d.initializeLegacySettings(p));
    assert(d.settings().speed==800.f && d.settings().sfx==1000.f && d.settings().music==800.f);
    assert(std::filesystem::file_size(p)==LegacySettingsTrace::fileSize);
    std::filesystem::remove(p,ec);
    std::cout<<"settings persistence r301 PASS\n";
}

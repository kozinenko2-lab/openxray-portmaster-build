#include "game/level.hpp"
#include "audio/legacy_sfx_bank.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#ifndef AIRXONIX_SOURCE_DIR
#define AIRXONIX_SOURCE_DIR "."
#endif
static bool check(bool v,const char* m){if(!v)std::cerr<<"FAIL: "<<m<<"\n";return v;}
static std::vector<std::uint8_t> read(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
int main(){
    namespace fs=std::filesystem;bool ok=true;const fs::path a=fs::path(AIRXONIX_SOURCE_DIR)/"assets";
    ok&=check(!fs::exists(a/"AirXonix.wrp.exe"),"clean-room source assets must not contain original EXE");
    auto si=read(a/"raw"/"SOUNDINF.bin");ok&=check(si.size()==0x1CA4u,"clean SOUNDINF size");
    const auto generated=makeCleanroomSoundInf();ok&=check(generated==si,"shipped SOUNDINF matches deterministic in-memory clean-room fallback");
    try{LevelDatabase db(si);ok&=check(db.totalLevels()==82u,"clean level count");const auto&r=db.level(0,0);ok&=check(r.enemySpeed==8&&r.enemyTypeACount==2&&r.crawlerCount==1,"clean first level contract");const auto&x=db.level(4,19);ok&=check(x.enemySpeed>=16&&x.crawlerSpeed>=16,"clean extreme progression");}catch(...){ok=false;std::cerr<<"FAIL: clean SOUNDINF decode\n";}
    auto wp=read(a/"raw"/"WAVEPACK.bin");ok&=check(wp.size()==416u,"clean WAVEPACK size");
    airxonix::LegacySfxBank bank;ok&=check(bank.load((a/"music"/"29.MUS").string(),wp),"clean SFX bank loads");ok&=check(bank.size()==891221u,"clean SFX bank exact compatibility length");ok&=check(bank.resolveLogicalId(0x02).valid&&bank.resolveLogicalId(0x24).valid,"clean SFX logical ids resolve");
    for(int i=0;i<10;++i){char n[16];std::snprintf(n,sizeof(n),"%02d.mus",i);ok&=check(fs::file_size(a/"music"/n)>100000u,"clean music track exists");}
    std::size_t tga=0;for(auto& e:fs::directory_iterator(a/"textures"))if(e.path().extension()==".tga")++tga;ok&=check(tga>=70u,"clean texture pack complete");
    return ok?0:1;
}

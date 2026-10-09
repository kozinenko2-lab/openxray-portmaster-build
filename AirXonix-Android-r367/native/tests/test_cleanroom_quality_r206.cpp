#include "render/legacy_atlas_runtime.hpp"
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#ifndef AIRXONIX_SOURCE_DIR
#define AIRXONIX_SOURCE_DIR "."
#endif
static bool check(bool v,const char* m){if(!v)std::cerr<<"FAIL: "<<m<<"\n";return v;}
static std::vector<std::uint8_t> read(const std::filesystem::path&p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
static double meanDiff(const std::vector<std::uint8_t>&a,const std::vector<std::uint8_t>&b){
    const std::size_t n=std::min(a.size(),b.size()); if(!n)return 0.0; double s=0;for(std::size_t i=0;i<n;++i)s+=std::abs(int(a[i])-int(b[i]));return s/double(n);
}
static bool centeredEdges(const std::vector<std::uint8_t>&p){
    if(p.size()<64)return false;
    for(std::size_t i=0;i<16;++i)if(std::abs(int(p[i])-128)>2)return false;
    for(std::size_t i=p.size()-16;i<p.size();++i)if(std::abs(int(p[i])-128)>2)return false;
    return true;
}
static std::size_t visible(const LegacyAtlasImage&i){std::size_t n=0;for(std::size_t p=3;p<i.rgba.size();p+=4)if(i.rgba[p]>32)++n;return n;}
int main(){
    namespace fs=std::filesystem;bool ok=true;const fs::path a=fs::path(AIRXONIX_SOURCE_DIR)/"assets";
    const auto menu=read(a/"music"/"00.mus"),info=read(a/"music"/"07.mus"),game=read(a/"music"/"03.mus");
    ok&=check(menu.size()>700000&&info.size()>700000&&game.size()>700000,"r206 music loops have full standalone payload");
    ok&=check(centeredEdges(menu)&&centeredEdges(info)&&centeredEdges(game),"music loop edges are click-safe");
    ok&=check(meanDiff(menu,info)>15.0,"Information music is materially different from main-menu music");
    ok&=check(meanDiff(game,menu)>12.0,"gameplay music is materially different from main-menu music");
    LegacyAtlasImage player,enemy;std::string err;
    ok&=check(LegacyAtlasRuntime::loadTexture((a/"textures").string(),"XONI",nullptr,player,&err),"clean XONI loads");
    ok&=check(LegacyAtlasRuntime::loadTexture((a/"textures").string(),"BALL",nullptr,enemy,&err),"clean BALL loads");
    ok&=check(player.width==32&&player.height==32&&enemy.width==32&&enemy.height==32,"player/enemy compatibility dimensions");
    ok&=check(visible(player)>250&&visible(enemy)>250,"player/enemy have substantial visible silhouettes");
    ok&=check(player.rgba!=enemy.rgba,"player/enemy art is visually distinct");
    std::ifstream mf(a/"cleanroom"/"MANIFEST.txt");std::string manifest((std::istreambuf_iterator<char>(mf)),{});
    ok&=check(manifest.find("r208")!=std::string::npos,"manifest identifies current clean-room pack");
    std::ifstream sf(a/"cleanroom"/"SHA256SUMS.txt");std::string line;std::size_t lines=0;bool entriesExist=true;
    while(std::getline(sf,line)){if(line.size()<67)continue;++lines;const std::string rel=line.substr(66);if(!fs::exists(a/rel))entriesExist=false;}
    ok&=check(lines==95,"SHA256 manifest covers all 95 pre-checksum resources");
    ok&=check(entriesExist,"SHA256 manifest references existing replacement resources");
    return ok?0:1;
}

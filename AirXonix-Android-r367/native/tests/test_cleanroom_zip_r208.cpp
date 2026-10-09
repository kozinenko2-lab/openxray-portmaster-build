#include "core/cleanroom_archive.hpp"
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
int main(){
    std::string error;
    const std::string path=std::string(AIRXONIX_SOURCE_DIR)+"/AirXonix-cleanroom.zip";
    auto& z=airxonix::CleanroomArchive::instance();
    if(!z.open(path,&error)){std::cerr<<error<<"\n";return 1;}
    if(z.entryCount()<90u)return 2;
    std::vector<std::uint8_t> data;
    if(!z.read("raw/SOUNDINF.bin",data)||data.size()!=0x1ca4u)return 3;
    if(!z.read("music/00.mus",data)||data.size()<1000u)return 4;
    if(!z.read("music/29.MUS",data)||data.size()!=891221u)return 5;
    if(!z.read("textures/LOGO.tga",data)||data.size()<18u)return 6;
    if(z.exists("../music/00.mus"))return 7;
    return 0;
}

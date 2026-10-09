#include "render/legacy_atlas_runtime.hpp"
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>

static bool check(bool v,const char* m){if(!v)std::cerr<<"FAIL: "<<m<<"\n";return v;}
int main(){
    namespace fs=std::filesystem;bool ok=true;
    const fs::path dir=fs::temp_directory_path()/"airxonix-r205-tga";fs::create_directories(dir);
    const fs::path p=dir/"TEST.tga";
    // 2x2 uncompressed BGRA TGA, top-left origin.
    std::array<std::uint8_t,18+16> b{};b[2]=2;b[12]=2;b[14]=2;b[16]=32;b[17]=0x28;
    const std::uint8_t px[16]={0,0,255,255, 0,255,0,255, 255,0,0,255, 255,255,255,128};
    for(int i=0;i<16;++i)b[18+i]=px[i];{std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),std::streamsize(b.size()));}
    LegacyAtlasImage im;std::string err;
    ok&=check(LegacyAtlasRuntime::loadTexture(dir.string(),"TEST",nullptr,im,&err),"TGA must load without original EXE");
    ok&=check(im.width==2&&im.height==2&&im.rgba.size()==16,"TGA dimensions/RGBA size");
    ok&=check(im.rgba[0]==255&&im.rgba[1]==0&&im.rgba[2]==0&&im.rgba[3]==255,"BGRA->RGBA/top-left conversion");
    ok&=check(im.rgba[12]==255&&im.rgba[13]==255&&im.rgba[14]==255&&im.rgba[15]==128,"TGA alpha preserved");
    fs::remove_all(dir);return ok?0:1;
}

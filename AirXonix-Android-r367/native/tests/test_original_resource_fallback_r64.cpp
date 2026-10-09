#include "core/legacy_original_resources.hpp"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

static void put32(std::vector<std::uint8_t>& b,std::size_t o,std::uint32_t v){for(int i=0;i<4;++i)b[o+i]=std::uint8_t(v>>(i*8));}
int main(){
    using airxonix::LegacyOriginalResourceTrace;
    const std::size_t bmpOff=LegacyOriginalResourceTrace::rsrcFileOffset+(LegacyOriginalResourceTrace::bmpPackRva-LegacyOriginalResourceTrace::rsrcRva);
    const std::size_t sndOff=LegacyOriginalResourceTrace::rsrcFileOffset+(LegacyOriginalResourceTrace::soundInfRva-LegacyOriginalResourceTrace::rsrcRva);
    const std::size_t wavOff=LegacyOriginalResourceTrace::rsrcFileOffset+(LegacyOriginalResourceTrace::wavePackRva-LegacyOriginalResourceTrace::rsrcRva);
    std::vector<std::uint8_t> exe(wavOff+LegacyOriginalResourceTrace::wavePackBytes,0);
    put32(exe,bmpOff+0,24); put32(exe,bmpOff+4,0x54455354u); // TEST
    put32(exe,bmpOff+8,2); put32(exe,bmpOff+12,2);
    // RGB565: red, green, blue, black.
    const std::uint16_t px[4]={0xF800,0x07E0,0x001F,0x0000};
    for(int i=0;i<4;++i){exe[bmpOff+16+i*2]=std::uint8_t(px[i]);exe[bmpOff+17+i*2]=std::uint8_t(px[i]>>8);}
    exe[sndOff]=0x5A; exe[wavOff]=0xA5;
    const std::string path="airxonix_fake_original_r64.exe";{std::ofstream f(path,std::ios::binary);f.write(reinterpret_cast<const char*>(exe.data()),std::streamsize(exe.size()));}
    airxonix::LegacyOriginalResources r;std::string error;assert(r.open(path,&error));assert(r.textureCount()==1);
    airxonix::LegacyPackedImage im;assert(r.loadTexture("TEST",im,&error));assert(im.width==2&&im.height==2&&im.rgba.size()==16);
    assert(im.rgba[0]>240&&im.rgba[1]<8&&im.rgba[2]<8&&im.rgba[3]==255);
    assert(im.rgba[4]<8&&im.rgba[5]>248&&im.rgba[6]<8&&im.rgba[7]==255);
    assert(im.rgba[8]<8&&im.rgba[9]<8&&im.rgba[10]>240&&im.rgba[11]==255);
    assert(im.rgba[15]==0);
    std::vector<std::uint8_t> data;assert(r.loadSoundInf(data,&error)&&data.size()==LegacyOriginalResourceTrace::soundInfBytes&&data[0]==0x5A);
    assert(r.loadWavePack(data,&error)&&data.size()==LegacyOriginalResourceTrace::wavePackBytes&&data[0]==0xA5);
    std::remove(path.c_str());return 0;
}

#include "legacy_original_resources.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>

namespace airxonix {
namespace {
std::uint16_t u16le(const std::uint8_t* p){return std::uint16_t(p[0])|std::uint16_t(p[1]<<8);}
std::uint32_t u32le(const std::uint8_t* p){return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);}
std::size_t fileOff(std::size_t rva){return LegacyOriginalResourceTrace::rsrcFileOffset+(rva-LegacyOriginalResourceTrace::rsrcRva);}
std::string decodeFourcc(std::uint32_t v){char s[5]{};s[0]=char((v>>24)&255);s[1]=char((v>>16)&255);s[2]=char((v>>8)&255);s[3]=char(v&255);return std::string(s,4);}
}

bool LegacyOriginalResources::open(const std::string& exePath,std::string* error){
    close();std::ifstream f(exePath,std::ios::binary);if(!f){if(error)*error="cannot open original executable: "+exePath;return false;}
    exe_.assign(std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>());path_=exePath;
    if(exe_.size()<fileOff(LegacyOriginalResourceTrace::bmpPackRva)+LegacyOriginalResourceTrace::bmpPackBytes){if(error)*error="original executable is truncated or unsupported";close();return false;}
    const std::size_t base=fileOff(LegacyOriginalResourceTrace::bmpPackRva),end=base+LegacyOriginalResourceTrace::bmpPackBytes;
    std::size_t p=base;
    while(p+16<=end){const auto size=u32le(exe_.data()+p);const auto id=u32le(exe_.data()+p+4);const auto w=u32le(exe_.data()+p+8);const auto h=u32le(exe_.data()+p+12);if(size==0)break;
        const std::size_t expected=16u+std::size_t(w)*std::size_t(h)*2u;if(!w||!h||size<expected||p+size>end){if(error)*error="invalid BMPPACK record in original executable";close();return false;}
        textures_[decodeFourcc(id)]={p+16,w,h};p+=size;
    }
    if(textures_.empty()){if(error)*error="BMPPACK contains no textures";close();return false;}return true;
}
void LegacyOriginalResources::close(){path_.clear();exe_.clear();textures_.clear();}

bool LegacyOriginalResources::loadTexture(std::string_view fourcc,LegacyPackedImage& out,std::string* error) const{
    out={};auto it=textures_.find(std::string(fourcc));if(it==textures_.end()){if(error)*error="BMPPACK texture not found: "+std::string(fourcc);return false;}
    const auto& r=it->second;out.width=int(r.width);out.height=int(r.height);out.rgba.resize(std::size_t(r.width)*r.height*4u);
    for(std::size_t i=0,n=std::size_t(r.width)*r.height;i<n;++i){const auto v=std::uint16_t(exe_[r.pixelOffset+i*2])|std::uint16_t(exe_[r.pixelOffset+i*2+1]<<8);const auto r5=(v>>11)&31,g6=(v>>5)&63,b5=v&31;
        auto* d=out.rgba.data()+i*4;d[0]=std::uint8_t((r5<<3)|(r5>>2));d[1]=std::uint8_t((g6<<2)|(g6>>4));d[2]=std::uint8_t((b5<<3)|(b5>>2));d[3]=(d[0]==0&&d[1]==0&&d[2]==0)?0:255;}
    return true;
}


bool LegacyOriginalResources::copyFixedResource(std::size_t offset,std::size_t size,std::vector<std::uint8_t>& out,std::string* error,const char* label) const{
    out.clear();if(exe_.empty()||offset+size>exe_.size()){if(error)*error=std::string(label)+" unavailable in original executable";return false;}out.assign(exe_.begin()+std::ptrdiff_t(offset),exe_.begin()+std::ptrdiff_t(offset+size));return true;
}
bool LegacyOriginalResources::loadSoundInf(std::vector<std::uint8_t>& out,std::string* error) const{return copyFixedResource(fileOff(LegacyOriginalResourceTrace::soundInfRva),LegacyOriginalResourceTrace::soundInfBytes,out,error,"SOUNDINF");}
bool LegacyOriginalResources::loadWavePack(std::vector<std::uint8_t>& out,std::string* error) const{return copyFixedResource(fileOff(LegacyOriginalResourceTrace::wavePackRva),LegacyOriginalResourceTrace::wavePackBytes,out,error,"WAVEPACK");}
}

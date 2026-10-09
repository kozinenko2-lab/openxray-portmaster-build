#include "cleanroom_archive.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>

namespace airxonix {
namespace {
std::uint16_t u16(const std::uint8_t* p){return std::uint16_t(p[0])|(std::uint16_t(p[1])<<8);}
std::uint32_t u32(const std::uint8_t* p){return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);}
constexpr std::uint32_t kEocd=0x06054b50u,kCentral=0x02014b50u,kLocal=0x04034b50u;
}

CleanroomArchive& CleanroomArchive::instance(){static CleanroomArchive v;return v;}
std::string CleanroomArchive::key(std::string s){for(char& c:s){if(c=='\\')c='/';c=char(std::tolower(static_cast<unsigned char>(c)));}while(s.rfind("./",0)==0)s.erase(0,2);while(!s.empty()&&s.front()=='/')s.erase(s.begin());return s;}
void CleanroomArchive::close(){path_.clear();entries_.clear();}

bool CleanroomArchive::open(const std::string& path,std::string* error){
    close();
    std::ifstream f(path,std::ios::binary);
    if(!f){if(error)*error="cannot open clean-room ZIP: "+path;return false;}
    f.seekg(0,std::ios::end);const auto end=f.tellg();if(end<std::streamoff(22)){if(error)*error="clean-room ZIP too small";return false;}
    const std::streamoff tailSize=std::min<std::streamoff>(end,65557);
    f.seekg(end-tailSize);std::vector<std::uint8_t> tail(static_cast<std::size_t>(tailSize),std::uint8_t{0});f.read(reinterpret_cast<char*>(tail.data()),tailSize);
    std::ptrdiff_t eocd=-1;for(std::ptrdiff_t i=std::ptrdiff_t(tail.size())-22;i>=0;--i)if(u32(tail.data()+i)==kEocd){eocd=i;break;}
    if(eocd<0){if(error)*error="ZIP EOCD not found";return false;}
    const auto* e=tail.data()+eocd;const std::uint16_t count=u16(e+10);const std::uint32_t centralSize=u32(e+12),centralOffset=u32(e+16);
    (void)centralSize;
    f.clear();f.seekg(std::streamoff(centralOffset));
    for(std::uint16_t n=0;n<count;++n){
        std::uint8_t h[46]{};f.read(reinterpret_cast<char*>(h),46);if(f.gcount()!=46||u32(h)!=kCentral){close();if(error)*error="invalid ZIP central directory";return false;}
        const std::uint16_t flags=u16(h+8),method=u16(h+10),nameLen=u16(h+28),extraLen=u16(h+30),commentLen=u16(h+32);
        const std::uint32_t csize=u32(h+20),usize=u32(h+24),local=u32(h+42);
        std::string name(nameLen,'\0');if(nameLen)f.read(name.data(),nameLen);f.seekg(std::streamoff(extraLen)+std::streamoff(commentLen),std::ios::cur);
        if(name.empty()||name.back()=='/')continue;
        entries_[key(name)]={local,csize,usize,method,flags};
    }
    if(entries_.empty()){close();if(error)*error="clean-room ZIP contains no files";return false;}
    path_=path;return true;
}

bool CleanroomArchive::exists(const std::string& name) const{return entries_.find(key(name))!=entries_.end();}
bool CleanroomArchive::read(const std::string& name,std::vector<std::uint8_t>& out) const{
    out.clear();if(path_.empty())return false;const auto it=entries_.find(key(name));if(it==entries_.end())return false;const Entry& e=it->second;
    if(e.method!=0 || (e.flags&1u) || e.compressedSize!=e.uncompressedSize)return false;
    std::ifstream f(path_,std::ios::binary);if(!f)return false;f.seekg(std::streamoff(e.localOffset));std::uint8_t h[30]{};f.read(reinterpret_cast<char*>(h),30);if(f.gcount()!=30||u32(h)!=kLocal)return false;
    const std::uint16_t nameLen=u16(h+26),extraLen=u16(h+28);f.seekg(std::streamoff(nameLen)+std::streamoff(extraLen),std::ios::cur);
    out.resize(e.uncompressedSize);if(!out.empty())f.read(reinterpret_cast<char*>(out.data()),std::streamsize(out.size()));if(f.gcount()!=std::streamsize(out.size())){out.clear();return false;}return true;
}

} // namespace airxonix

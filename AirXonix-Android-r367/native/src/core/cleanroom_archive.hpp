#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace airxonix {

// Read-only ZIP VFS for the clean-room resource pack. Runtime packages create
// this archive with ZIP method 0 (stored): decoding therefore needs no zlib and
// introduces no additional firmware dependency on H700/RK3326 devices.
class CleanroomArchive {
public:
    static CleanroomArchive& instance();
    bool open(const std::string& path,std::string* error=nullptr);
    void close();
    bool isOpen() const{return !path_.empty();}
    const std::string& path() const{return path_;}
    bool exists(const std::string& name) const;
    bool read(const std::string& name,std::vector<std::uint8_t>& out) const;
    std::size_t entryCount() const{return entries_.size();}
private:
    struct Entry {std::uint32_t localOffset=0,compressedSize=0,uncompressedSize=0;std::uint16_t method=0,flags=0;};
    static std::string key(std::string s);
    std::string path_;
    std::unordered_map<std::string,Entry> entries_;
};

} // namespace airxonix

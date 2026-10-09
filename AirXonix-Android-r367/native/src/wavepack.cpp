#include "wavepack.hpp"
#include <stdexcept>
std::vector<WavePackEntry> parseWavePack(const std::vector<std::uint8_t>& d) {
    if (d.size() % 8) throw std::runtime_error("WAVEPACK size is not a multiple of 8");
    std::vector<WavePackEntry> out;
    out.reserve(d.size()/8);
    for (std::size_t p=0; p<d.size(); p+=8) {
        std::string id;
        id.push_back(static_cast<char>(d[p+3])); id.push_back(static_cast<char>(d[p+2]));
        id.push_back(static_cast<char>(d[p+1])); id.push_back(static_cast<char>(d[p+0]));
        std::uint32_t v = static_cast<std::uint32_t>(d[p+4]) |
                          (static_cast<std::uint32_t>(d[p+5])<<8) |
                          (static_cast<std::uint32_t>(d[p+6])<<16) |
                          (static_cast<std::uint32_t>(d[p+7])<<24);
        out.push_back({id,v});
    }
    return out;
}

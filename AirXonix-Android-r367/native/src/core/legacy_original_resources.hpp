#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace airxonix {

struct LegacyPackedImage {
    int width=0;
    int height=0;
    std::vector<std::uint8_t> rgba;
};

class LegacyOriginalResources {
public:
    bool open(const std::string& exePath,std::string* error=nullptr);
    void close();
    bool isOpen() const { return !exe_.empty(); }
    const std::string& path() const { return path_; }
    bool loadTexture(std::string_view fourcc,LegacyPackedImage& out,std::string* error=nullptr) const;
    bool loadSoundInf(std::vector<std::uint8_t>& out,std::string* error=nullptr) const;
    bool loadWavePack(std::vector<std::uint8_t>& out,std::string* error=nullptr) const;
    std::size_t textureCount() const { return textures_.size(); }
private:
    struct TextureRecord { std::size_t pixelOffset{}; std::uint32_t width{},height{}; };
    bool copyFixedResource(std::size_t offset,std::size_t size,std::vector<std::uint8_t>& out,std::string* error,const char* label) const;
    std::string path_;
    std::vector<std::uint8_t> exe_;
    std::unordered_map<std::string,TextureRecord> textures_;
};

struct LegacyOriginalResourceTrace {
    static constexpr std::size_t rsrcFileOffset=0x00046000u;
    static constexpr std::size_t rsrcRva=0x021BA000u;
    static constexpr std::size_t bmpPackRva=0x021BA570u;
    static constexpr std::size_t bmpPackBytes=0x001157E0u;
    static constexpr std::size_t soundInfRva=0x022CFD50u;
    static constexpr std::size_t soundInfBytes=0x00001CA4u;
    static constexpr std::size_t wavePackRva=0x022D19F4u;
    static constexpr std::size_t wavePackBytes=0x000001A0u;
};

}

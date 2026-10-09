#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include "legacy_spatial_audio.hpp"

namespace airxonix {

struct LegacyWavePackDirectoryEntry {
    std::string_view fourcc;
    std::uint32_t rawLength;
};

// DIRECT EXE/resource r56. WAVEPACK/BIN contains 52 directory records
// (50 real samples, one 0xFFFFFFFF sentinel and one zero terminator).
// 0x409BE0 maps music\\29.mus and 0x409C51 walks records in-order, assigning
// each sample pointer to the current bank cursor and then advancing by rawLength.
struct LegacyWavePackDirectoryTrace {
    static constexpr std::uintptr_t loader = 0x00409BE0u;
    static constexpr std::uintptr_t walkBegin = 0x00409C51u;
    static constexpr std::uintptr_t lookup = 0x00409CC0u;
    static constexpr std::size_t resourceBytes = 416u;
    static constexpr std::size_t recordBytes = 8u;
    static constexpr std::size_t recordCount = 52u;
    static constexpr std::size_t realSampleCount = 50u;
    static constexpr std::uint32_t sentinelLength = 512u;

    static constexpr std::array<LegacyWavePackDirectoryEntry, realSampleCount> entries{{
        {"haha",46169},{"bon3",23136},{"min0",2560},{"fir1",11482},{"up01",15799},
        {"levl",53263},{"bon1",18082},{"comp",75595},{"bvzr",10827},{"fire",21222},
        {"game",53235},{"bol2",1988},{"bon0",12003},{"bon2",22426},{"bol1",654},
        {"clc1",1352},{"clc2",1480},{"bfly",34080},{"vint",40855},{"sfly",5543},
        {"gove",17795},{"efly",15175},{"bpop",8128},{"fir2",29076},{"tick",20047},
        {"bonu",14577},{"cow2",11593},{"cow4",11977},{"cow1",6062},{"beep",3026},
        {"cow3",12965},{"time",13456},{"bour",1843},{"welk",17868},{"slow",16315},
        {"life",12586},{"strt",17299},{"byeb",13844},{"acce",20275},{"lets",22287},
        {"aaaa",19334},{"that",23675},{"out!",13238},{"ohoh",11891},{"oyoy",4278},
        {"cool",9882},{"sur1",22646},{"whip",12512},{"cmex",22303},{"yes1",13517}
    }};
};

struct LegacySfxSampleView {
    std::size_t offset{};
    std::size_t rawLength{};
    std::size_t playbackLength{};
    bool valid{};
};

class LegacySfxBank {
public:
    bool load(const std::string& path);
    bool load(const std::string& path,const std::vector<std::uint8_t>& wavePackDirectory);
    bool loadBytes(std::vector<std::uint8_t> bytes);
    bool loadBytes(std::vector<std::uint8_t> bytes,const std::vector<std::uint8_t>& wavePackDirectory);
    void clear();
    bool loaded() const { return !bytes_.empty(); }
    std::size_t size() const { return bytes_.size(); }
    LegacySfxSampleView findFourcc(std::string_view fourcc) const;
    LegacySfxSampleView resolveLogicalId(std::size_t logicalId) const;
    const std::uint8_t* dataAt(std::size_t offset) const;
private:
    struct OwnedEntry { std::string fourcc; std::uint32_t rawLength{}; };
    std::vector<std::uint8_t> bytes_;
    std::vector<OwnedEntry> directory_;
};

} // namespace airxonix

namespace airxonix {

// Native-safe seven-voice compatibility mixer. Pool policy follows the x86
// first-free allocator, but exhaustion rejects instead of reproducing its
// out-of-bounds eighth-slot bug. Sample bytes are mono unsigned-8 bank data;
// output is unsigned-8 stereo at the legacy 22050-Hz device rate.
class NativeLegacySfxMixer {
public:
    static constexpr std::size_t kVoiceCount = 7u;
    struct Voice { LegacySfxSampleView sample{}; std::size_t cursor{}; int gainL{63}; int gainR{63}; bool active{}; };
    explicit NativeLegacySfxMixer(const LegacySfxBank* bank=nullptr):bank_(bank){}
    void setBank(const LegacySfxBank* bank){bank_=bank; stopAll();}
    int playLogicalId(std::size_t logicalId, int gainL=63, int gainR=63);
    int playSpatialLogicalId(std::size_t logicalId,float x,float y,float z,float scalar);
    bool updateSpatial(int handle,float x,float y,float z,float scalar=1.f);
    void setSpatialListener(float x,float y,float z){listener_.x=x;listener_.y=y;listener_.z=z;}
    void setSpatialBasis(const LegacySpatialBasis& basis){listener_.basis=basis;}
    void setMasterScale(float scale){masterScale_=scale<0.f?0.f:(scale>1.f?1.f:scale);}
    float masterScale() const{return masterScale_;}
    const LegacySpatialListener& spatialListener() const{return listener_;}
    void stop(int handle);
    void stopAll();
    void mixStereoCentered(std::int32_t* dst, std::size_t frames);
    void mixStereoU8(std::uint8_t* dst, std::size_t frames);
    const std::array<Voice,kVoiceCount>& voices() const { return voices_; }
private:
    const LegacySfxBank* bank_{};
    std::array<Voice,kVoiceCount> voices_{};
    LegacySpatialListener listener_{0.f,0.f,0.f,legacyBuildAudioBasis(0,-302,0)};
    float masterScale_=1.f;
};

} // namespace airxonix

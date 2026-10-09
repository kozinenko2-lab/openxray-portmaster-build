#pragma once
#include "legacy_audio_trace.hpp"
#include "legacy_output_quantizer.hpp"
#include <algorithm>
#include <cstdint>

namespace airxonix {

// DIRECT EXE r148: 0x40B1EB..0x40B22A computes integer music channel gains as
// trunc(master * stereo * transitionGain * 63.0). 0x40A890 then sends music
// bytes through the same 0x457A54 gain table into the same 32-bit accumulator
// used by SFX, and 0x40AA80 saturates once after all sources are present.
struct LegacyUnifiedMixTrace {
    static constexpr std::uint32_t musicGainRoutine=0x0040B0D0u;
    static constexpr std::uint32_t mainMixer=0x0040A890u;
    static constexpr std::uint32_t outputQuantizer=0x0040AA80u;
    static constexpr int gainScale=63;
    static int musicGain(float master,float transition,float stereo){
        const int g=static_cast<int>(master*transition*stereo*float(gainScale));
        return std::clamp(g,0,static_cast<int>(LegacyGainTableTrace::gainRows-1));
    }
    static constexpr int contribution(int gain,std::uint8_t sample){
        return LegacyGainTableTrace::contribution(gain,int(sample));
    }
    static constexpr std::uint8_t finalize(int centeredSum){
        return LegacyOutputQuantizerTrace::quantizeCenteredSum(centeredSum);
    }
};

} // namespace airxonix

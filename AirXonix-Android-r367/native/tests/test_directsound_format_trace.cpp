#include "audio/legacy_audio_trace.hpp"
#include <cassert>
using namespace airxonix;
int main(){
    static_assert(LegacyDirectSoundFormatTrace::formatTag == 1u);
    static_assert(LegacyDirectSoundFormatTrace::channels == 2u);
    static_assert(LegacyDirectSoundFormatTrace::samplesPerSecond == 22050u);
    static_assert(LegacyDirectSoundFormatTrace::averageBytesPerSecond == 44100u);
    static_assert(LegacyDirectSoundFormatTrace::blockAlign == 2u);
    static_assert(LegacyDirectSoundFormatTrace::bitsPerSample == 8u);
    static_assert(LegacyDirectSoundFormatTrace::cbSize == 0u);
    assert(LegacyDirectSoundFormatTrace::formatBuildBegin == 0x00409F51u);
    assert(LegacyDirectSoundFormatTrace::formatBuildEnd == 0x00409F9Bu);
    return 0;
}

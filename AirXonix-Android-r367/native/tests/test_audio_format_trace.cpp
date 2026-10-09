#include "audio/legacy_audio_trace.hpp"
#include "game/level.hpp"
#include <cassert>
using namespace airxonix;
int main(){
    static_assert(LegacySfxMixerTrace::voiceStride==0x20u);
    static_assert(LegacySfxMixerTrace::sampleBaseOffset==0x00u);
    static_assert(LegacySfxMixerTrace::sampleEndOffset==0x04u);
    static_assert(LegacySfxMixerTrace::cursorOffset==0x08u);
    static_assert(LegacySfxMixerTrace::worldXOffset==0x10u);
    static_assert(LegacySfxMixerTrace::worldYOffset==0x14u);
    static_assert(LegacySfxMixerTrace::worldZOffset==0x18u);
    static_assert(LegacySfxMixerTrace::attenuationOffset==0x1Cu);
    assert(!LegacySfxMixerTrace::hasVoiceStealing);
    assert(LegacySfxMixerTrace::legacyOverflowIndex==7u);
    assert(LegacySfxMixerTrace::legacyOverflowHandle==8);
    assert(LegacyMusicStreamTrace::mapOpenRoutine==0x004097C0u);
    assert(!LegacyMusicStreamTrace::parsesContainerHeader);
    assert(LegacyMusicStreamTrace::loopsAtMappedEnd);
    static_assert(LegacyModeResourceTrace::physicalRecordCapacity==256u);
    static_assert(LegacyModeResourceTrace::opaqueTailRecordRegions==174u);
    return 0;
}

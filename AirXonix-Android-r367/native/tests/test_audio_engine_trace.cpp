#include "audio/legacy_audio_trace.hpp"
#include <cassert>
using namespace airxonix;
int main(){
    static_assert(LegacySfxMixerTrace::voiceCount==7u);
    static_assert(LegacySfxMixerTrace::voiceStride==0x20u);
    assert(LegacySfxMixerTrace::voicePool==0x00451548u);
    assert(LegacySfxMixerTrace::voiceAllocator==0x0040AEF0u);
    assert(LegacySfxMixerTrace::mixerRoutine==0x0040A890u);
    assert(LegacySfxMixerTrace::simpleGainScale==63);
    assert(LegacySfxMixerTrace::spatialGainClamp==0x55);
    assert(LegacyMusicStreamTrace::openTrack==0x0040AFC0u);
    assert(LegacyMusicStreamTrace::updateFade==0x0040B0D0u);
    assert(LegacyMusicStreamTrace::requestTrack==0x0040B240u);
    assert(LegacyMusicStreamTrace::beginFadeOut==0x0040B260u);
    assert(LegacyMusicStreamTrace::probeStrideBytes==0x400u);
    assert(LegacyMusicStreamTrace::mappedObjectDataOffset==0x10u);
    assert(LegacyMusicStreamTrace::mappedObjectLengthOffset==0x14u);
    return 0;
}

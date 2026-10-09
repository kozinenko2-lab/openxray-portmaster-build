#include "audio/legacy_audio_trace.hpp"
#include "game/legacy_effects.hpp"
#include <cassert>
using namespace airxonix;
int main(){
    static_assert(LegacyDirectSoundFormatTrace::samplesPerSecond==22050u);
    static_assert(LegacyDirectSoundFormatTrace::channels==2u);
    static_assert(LegacyDirectSoundFormatTrace::bitsPerSample==8u);
    static_assert(!LegacySfxMixerTrace::reservedWrittenByAllocator);
    static_assert(!LegacySfxMixerTrace::reservedWrittenByUpdater);
    static_assert(!LegacySfxMixerTrace::reservedReadByMixer);
    assert(kLegacySharedBillboardInheritedState.textureHandle==3);
    assert(!kLegacySharedBillboardInheritedState.alphaBlend);
    assert(kLegacySharedBillboardInheritedState.gameplayTexture3==0x0041A5F6u);
    assert(kLegacySharedBillboardInheritedState.deathEraserCall==0x0041CDC0u);
}

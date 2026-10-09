#include "audio/legacy_audio_trace.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace airxonix;
int main(){
    static_assert(LegacyDeathAudioTrace::deathRoutine==0x0041C030u);
    static_assert(LegacyDeathAudioTrace::splitAtFinalSecond==0x0041C540u);
    static_assert(LegacyDeathAudioTrace::respawnVoiceStart==0x0041C569u);
    static_assert(LegacyDeathAudioTrace::respawnVoiceUpdate==0x0041C60Fu);
    static_assert(LegacyDeathAudioTrace::zeroLivesOneShot==0x0041C638u);
    static_assert(LegacyDeathAudioTrace::zeroLivesMusicFade==0x0041C644u);
    static_assert(LegacyDeathAudioTrace::respawnVoiceHandle==0x0257F4E8u);
    static_assert(LegacyDeathAudioTrace::respawnLogicalSfxId==7u);
    static_assert(LegacyDeathAudioTrace::zeroLivesLogicalSfxId==0x18u);
    assert(LegacySfxTrace::fourcc[LegacyDeathAudioTrace::respawnLogicalSfxId]==LegacyDeathAudioTrace::respawnFourcc);
    assert(LegacySfxTrace::fourcc[LegacyDeathAudioTrace::zeroLivesLogicalSfxId]==LegacyDeathAudioTrace::zeroLivesFourcc);
    assert(std::fabs(LegacyDeathAudioTrace::respawnInitialY-0.1f)<1e-7f);
    assert(std::fabs(LegacyDeathAudioTrace::zeroLivesMusicFadePerMs-0.0003f)<1e-8f);
    std::cout << "death audio r96 ok\\n";
}

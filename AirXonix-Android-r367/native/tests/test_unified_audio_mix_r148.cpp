#include "audio/legacy_music_mix.hpp"
#include <cassert>
using namespace airxonix;
int main(){
    assert(LegacyUnifiedMixTrace::musicGain(1.f,1.f,1.f)==63);
    assert(LegacyUnifiedMixTrace::musicGain(.85f,1.f,1.f)==53); // truncation, not rounding
    assert(LegacyUnifiedMixTrace::contribution(63,255)==125);
    assert(LegacyUnifiedMixTrace::contribution(50,0)==-100);
    // One final saturation after all sources: loud SFX can be pulled back below
    // the rail by opposite-polarity music. Pre-clipping SFX would incorrectly
    // yield 155 here (127-100+128) instead of 228.
    const int loudSfx=200;
    const int music=-100;
    assert(LegacyUnifiedMixTrace::finalize(loudSfx+music)==228);
    assert(LegacyUnifiedMixTrace::finalize(loudSfx)==255);
}

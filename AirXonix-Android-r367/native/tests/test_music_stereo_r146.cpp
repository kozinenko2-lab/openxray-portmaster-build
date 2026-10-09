#include "audio/legacy_audio_trace.hpp"
#include <cassert>
#include <cmath>
static bool near(float a,float b,float e=1e-6f){return std::fabs(a-b)<=e;}
int main(){
    using T=airxonix::LegacyMusicStereoTrace;
    static_assert(T::setter==0x0040B280u);
    static_assert(T::gameplayCaller==0x004195C4u);
    static_assert(T::finaleCaller==0x0041B7D3u);
    assert(near(T::channel0(0.5f),0.8f));
    assert(near(T::channel1(0.5f),0.8f));
    assert(near(T::channel0(0.4f),0.5f));
    assert(near(T::channel1(0.4f),1.1f));
    assert(near(T::channel0(0.6f),1.1f));
    assert(near(T::channel1(0.6f),0.5f));
    return 0;
}

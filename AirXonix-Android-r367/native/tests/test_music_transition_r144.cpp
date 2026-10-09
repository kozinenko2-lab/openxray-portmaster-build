#include "audio/legacy_music_state.hpp"
#include "audio/legacy_audio_trace.hpp"
#include <cassert>
#include <cmath>

static bool near(float a,float b,float e=1e-7f){return std::fabs(a-b)<=e;}
int main(){
    using airxonix::LegacyMusicTransitionState;
    using airxonix::LegacyMusicStreamTrace;
    static_assert(LegacyMusicStreamTrace::requestTrack==0x0040B240u);
    static_assert(LegacyMusicStreamTrace::beginFadeOut==0x0040B260u);
    static_assert(LegacyMusicStreamTrace::updateFade==0x0040B0D0u);

    LegacyMusicTransitionState s;s.reset(true,1.0f);
    s.requestTrack(3,0.002f);s.beginFadeOut(0.001f); // fade-out cancels older pending
    assert(s.pendingTrack()==-1);assert(near(s.fadeRatePerMs(),-0.001f));
    s.requestTrack(7,0.0005f); // r143 finale order: fade then request
    assert(s.pendingTrack()==7);

    // Strict boundary: exact zero remains active; only crossing below zero clamps/stops.
    auto r=s.update(1000,true);assert(!r.pending);assert(s.streamActive());assert(near(s.currentGain(),0.0f,2e-6f));
    r=s.update(1,true);assert(!r.pending);assert(!s.streamActive());assert(near(s.currentGain(),0.0f));

    // Switching occurs on the following inactive update.
    r=s.update(0,true);assert(r.pending&&r.trackId==7u&&near(r.fadeInPerMs,0.0005f));
    s.completeSwitch(true);assert(s.streamActive());assert(s.pendingTrack()==-1);assert(near(s.fadeRatePerMs(),0.0005f));

    // Exact one is not clamped/reset until it crosses above one.
    s.update(2000,true);assert(near(s.currentGain(),1.0f,2e-6f));assert(near(s.fadeRatePerMs(),0.0005f));
    s.update(1,true);assert(near(s.currentGain(),1.0f));assert(near(s.fadeRatePerMs(),0.0f));

    // Failed open still consumes the pending request exactly once.
    s.beginFadeOut(1.0f);s.requestTrack(4,0.1f);s.update(2,true);r=s.update(0,true);assert(r.pending&&r.trackId==4u);s.completeSwitch(false);assert(s.pendingTrack()==-1);assert(!s.streamActive());
    return 0;
}

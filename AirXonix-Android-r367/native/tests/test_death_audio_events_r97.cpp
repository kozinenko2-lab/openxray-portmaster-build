#include "core/legacy_highscore.hpp"
#define private public
#include "game/game.hpp"
#undef private
#include "audio/legacy_audio_trace.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using airxonix::LegacyDeathAudioTrace;
using airxonix::LegacySfxTrace;

static bool nearf(float a,float b,float eps=1e-6f){return std::fabs(a-b)<=eps;}

int main(){
    static_assert(LegacyDeathAudioTrace::entryLogicalSfxId==0x05u);
    static_assert(LegacyDeathAudioTrace::retainedLogicalSfxId==0x11u);
    static_assert(LegacyDeathAudioTrace::impactLogicalSfxId==0x04u);
    assert(LegacySfxTrace::fourcc[0x05]=="fir1");
    assert(LegacySfxTrace::fourcc[0x11]=="fir2");
    assert(LegacySfxTrace::fourcc[0x04]=="fire");

    Game g;
    g.lives_=2;
    g.player_.reset();
    g.handleDeath();
    auto ev=g.takeDeathAudioEvents();
    assert(ev.size()==2);
    assert(ev[0].kind==DeathAudioEventKind::SpatialPlay);
    assert(ev[0].logicalId==0x05u);
    assert(nearf(ev[0].scalar,1.f));
    assert(ev[1].kind==DeathAudioEventKind::SpatialStart);
    assert(ev[1].voice==DeathAudioVoiceTag::InitialDeathVoice);
    assert(ev[1].logicalId==0x11u);

    // Advance until the exact Y<.008 burst transition. It must stop the retained
    // fir2 voice and emit the spatial fire impact at scalar 1.3.
    bool sawBurst=false;
    for(int i=0;i<400 && !sawBurst;++i){
        g.updateDeathSequence(10);
        auto frame=g.takeDeathAudioEvents();
        if(g.deathScene_.triColorBurstStarted){
            assert(frame.size()>=2);
            bool sawStop=false,sawImpact=false;
            for(const auto& e:frame){
                if(e.kind==DeathAudioEventKind::SpatialStop && e.voice==DeathAudioVoiceTag::InitialDeathVoice) sawStop=true;
                if(e.kind==DeathAudioEventKind::SpatialPlay && e.logicalId==0x04u){
                    sawImpact=true; assert(nearf(e.scalar,1.2999999523162842f,1e-7f));
                }
            }
            assert(sawStop && sawImpact);
            sawBurst=true;
        }
    }
    assert(sawBurst);

    // Reach the final second with one life left: retained vint starts and then
    // receives positional updates while the presentation descends.
    bool sawRespawnStart=false,sawRespawnUpdate=false;
    while(g.deathScene_.remainingMs()>=1000){
        g.updateDeathSequence(10);
        auto frame=g.takeDeathAudioEvents();
        for(const auto& e:frame){
            if(e.kind==DeathAudioEventKind::SpatialStart && e.voice==DeathAudioVoiceTag::RespawnVoice && e.logicalId==0x07u) sawRespawnStart=true;
            if(e.kind==DeathAudioEventKind::SpatialUpdate && e.voice==DeathAudioVoiceTag::RespawnVoice) sawRespawnUpdate=true;
        }
    }
    for(int i=0;i<4;++i){
        g.updateDeathSequence(10);
        auto frame=g.takeDeathAudioEvents();
        for(const auto& e:frame){
            if(e.kind==DeathAudioEventKind::SpatialStart && e.voice==DeathAudioVoiceTag::RespawnVoice && e.logicalId==0x07u) sawRespawnStart=true;
            if(e.kind==DeathAudioEventKind::SpatialUpdate && e.voice==DeathAudioVoiceTag::RespawnVoice) sawRespawnUpdate=true;
        }
    }
    assert(sawRespawnStart && sawRespawnUpdate);

    // Zero-lives branch: simple haha + music fade are one-shot events.
    Game z;
    z.lives_=1;
    z.handleDeath();
    (void)z.takeDeathAudioEvents();
    z.updateDeathSequence(3001);
    auto ze=z.takeDeathAudioEvents();
    bool sawHaha=false,sawFade=false;
    for(const auto& e:ze){
        if(e.kind==DeathAudioEventKind::SimplePlay && e.logicalId==0x18u) sawHaha=true;
        if(e.kind==DeathAudioEventKind::MusicFadeOut){sawFade=true;assert(nearf(e.fadePerMs,0.0003000000142492354f,1e-9f));}
    }
    assert(sawHaha && sawFade);
    z.updateDeathSequence(10);
    ze=z.takeDeathAudioEvents();
    for(const auto& e:ze){
        assert(!(e.kind==DeathAudioEventKind::SimplePlay && e.logicalId==0x18u));
        assert(e.kind!=DeathAudioEventKind::MusicFadeOut);
    }

    std::cout << "death audio event choreography r97 ok\n";
}

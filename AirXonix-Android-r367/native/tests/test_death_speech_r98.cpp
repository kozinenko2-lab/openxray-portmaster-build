#include <filesystem>
#define private public
#include "game/game.hpp"
#undef private
#include "audio/legacy_audio_trace.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using airxonix::LegacyDeathSpeechTrace;
using airxonix::LegacySfxTrace;

static bool nearf(float a,float b,float eps=1e-6f){return std::fabs(a-b)<=eps;}

int main(){
    static_assert(LegacyDeathSpeechTrace::speechEnableGlobal==0x025B7898u);
    static_assert(LegacyDeathSpeechTrace::speechCycleGlobal==0x0257F548u);
    static_assert(LegacyDeathSpeechTrace::earlyThresholdRemainingMs==3850);
    static_assert(LegacyDeathSpeechTrace::gameOverLogicalId==0x1Fu);
    assert(LegacySfxTrace::fourcc[0x27]=="out!");
    assert(LegacySfxTrace::fourcc[0x2B]=="aaaa");
    assert(LegacySfxTrace::fourcc[0x2C]=="ohoh");
    assert(LegacySfxTrace::fourcc[0x34]=="oyoy");
    assert(LegacySfxTrace::fourcc[0x1F]=="gove");

    Game g;
    g.lives_=4;
    g.setSpeechEnabled(true);
    g.player_.reset();
    g.handleDeath();
    (void)g.takeDeathAudioEvents();

    // Equality does not fire: original CMP 0xF0A / JGE skips at exactly 3850.
    g.updateDeathSequence(150);
    auto ev=g.takeDeathAudioEvents();
    for(const auto& e:ev) assert(e.logicalId!=0x2Bu);
    assert(!g.deathScene_.earlySpeechCueConsumed);

    // First enabled cue: zero-initialized selector advances 0 -> 1, therefore aaaa.
    g.updateDeathSequence(1);
    ev=g.takeDeathAudioEvents();
    bool sawAaaa=false;
    for(const auto& e:ev){
        if(e.logicalId==0x2Bu){
            sawAaaa=true;
            assert(e.kind==DeathAudioEventKind::SpatialPlay);
            assert(nearf(e.scalar,1.5f));
        }
    }
    assert(sawAaaa);
    assert(g.deathScene_.earlySpeechCueConsumed);
    assert(g.deathSpeechCycleIndex_==1u);

    // Disabled speech consumes the one-shot window but must NOT advance selector.
    g.setSpeechEnabled(false);
    g.lives_=4;
    g.handleDeath();
    (void)g.takeDeathAudioEvents();
    g.updateDeathSequence(151);
    ev=g.takeDeathAudioEvents();
    for(const auto& e:ev) assert(e.logicalId!=0x27u && e.logicalId!=0x2Bu && e.logicalId!=0x2Cu && e.logicalId!=0x34u);
    assert(g.deathScene_.earlySpeechCueConsumed);
    assert(g.deathSpeechCycleIndex_==1u);

    // Re-enable: selector advances 1 -> 2, so next spoken death cue is ohoh.
    g.setSpeechEnabled(true);
    g.lives_=4;
    g.handleDeath();
    (void)g.takeDeathAudioEvents();
    g.updateDeathSequence(151);
    ev=g.takeDeathAudioEvents();
    bool sawOhoh=false;
    for(const auto& e:ev) if(e.kind==DeathAudioEventKind::SpatialPlay && e.logicalId==0x2Cu) sawOhoh=true;
    assert(sawOhoh);
    assert(g.deathSpeechCycleIndex_==2u);

    // Game Over tail: same speech flag gates one simple gove event at tail entry.
    g.deathScene_={};
    g.setSpeechEnabled(true);
    g.beginGameOverTail();
    ev=g.takeDeathAudioEvents();
    bool sawGove=false;
    for(const auto& e:ev) if(e.kind==DeathAudioEventKind::SimplePlay && e.logicalId==0x1Fu) sawGove=true;
    assert(sawGove);

    Game muted;
    muted.setSpeechEnabled(false);
    muted.beginGameOverTail();
    ev=muted.takeDeathAudioEvents();
    for(const auto& e:ev) assert(!(e.kind==DeathAudioEventKind::SimplePlay && e.logicalId==0x1Fu));

    std::cout << "death speech gate/cycle r98 ok\n";
}

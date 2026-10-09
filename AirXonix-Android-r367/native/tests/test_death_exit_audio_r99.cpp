#include <filesystem>
#define private public
#include "game/game.hpp"
#undef private
#include "audio/legacy_audio_trace.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using airxonix::LegacyDeathExitAudioTrace;
using airxonix::LegacySfxTrace;

static bool nearf(float a,float b,float eps=1e-6f){return std::fabs(a-b)<=eps;}

int main(){
    static_assert(LegacyDeathExitAudioTrace::respawnExitLogicalId==0x09u);
    static_assert(LegacyDeathExitAudioTrace::gameOverEventLogicalId==0x16u);
    assert(LegacySfxTrace::fourcc[0x09]=="efly");
    assert(LegacySfxTrace::fourcc[0x16]=="clc2");
    assert(!LegacyDeathExitAudioTrace::timeoutPlaysExitSfx);

    // Normal respawn completion emits efly and then retained-vint stop.
    Game g;
    g.lives_=2;
    g.player_.reset();
    g.handleDeath();
    (void)g.takeDeathAudioEvents();
    g.updateDeathSequence(4001);
    auto ev=g.takeDeathAudioEvents();
    bool sawEfly=false,sawStop=false;
    for(const auto& e:ev){
        if(e.kind==DeathAudioEventKind::SpatialPlay && e.logicalId==0x09u){
            sawEfly=true; assert(nearf(e.scalar,1.0f));
        }
        if(e.kind==DeathAudioEventKind::SpatialStop && e.voice==DeathAudioVoiceTag::RespawnVoice) sawStop=true;
    }
    assert(sawEfly && sawStop);
    assert(g.phase_==GamePhase::Gameplay);

    // Event-driven Game Over exit emits clc2.
    Game e;
    e.setSpeechEnabled(false); // keep gove out of this assertion set
    e.beginGameOverTail();
    (void)e.takeDeathAudioEvents();
    InputState none{};
    e.updateGameOverTail(none,1); // arm after release
    InputState press{}; press.action=true;
    e.updateGameOverTail(press,0);
    ev=e.takeDeathAudioEvents();
    bool sawClc2=false;
    for(const auto& x:ev) if(x.kind==DeathAudioEventKind::SimplePlay && x.logicalId==0x16u) sawClc2=true;
    assert(sawClc2);
    assert(e.phase_==GamePhase::Records);

    // Timeout exit bypasses 0x41CE87, so no clc2.
    Game t;
    t.setSpeechEnabled(false);
    t.beginGameOverTail();
    (void)t.takeDeathAudioEvents();
    t.gameOverScene_.inputArmed=true;
    t.updateGameOverTail(none,t.gameOverScene_.timeoutMs);
    assert(t.phase_==GamePhase::GameOver);
    t.updateGameOverTail(none,1);
    ev=t.takeDeathAudioEvents();
    for(const auto& x:ev) assert(!(x.kind==DeathAudioEventKind::SimplePlay && x.logicalId==0x16u));
    assert(t.phase_==GamePhase::Records);

    std::cout << "death terminal audio r99 ok\n";
}

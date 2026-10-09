#include <cassert>
#include <cmath>
#include <iostream>
#include "game/game.hpp"
#include "game/legacy_low_time_trace.hpp"

struct GameTestProbe {
    static void ready(Game& g){g.levelIntroScene_.elapsedMs=g.levelIntroScene_.durationMs; g.levelEntryScene_.running=false;}
    static void setTimer(Game& g,int v){g.timer_=v;}
    static void setScore(Game& g,int v){g.score_=v;}
    static bool warning(const Game& g){return g.lowTimeWarningActive_;}
    static int decayAccum(const Game& g){return g.scoreDecayAccumulatorMs_;}
    static void setDecayAccum(Game& g,int v){g.scoreDecayAccumulatorMs_=v;}
    static void pressure(Game& g,int dt){g.updateLegacyTimerPressure(dt);}
};

static bool near(float a,float b){return std::fabs(a-b)<1e-6f;}

int main(){
    const auto&t=kLegacyLowTimeTrace;
    assert(t.warningThreshold==0x2AF8);
    assert(t.warningLogicalSfxId==0x12u);
    assert(t.scoreDecayPeriodMs==1000 && t.scoreDecayPoints==50);
    assert(near(t.timerToSourceZ,1.9999999494757503e-5f));

    Game g; GameTestProbe::ready(g);
    InputState in{};
    GameTestProbe::setTimer(g,11001);
    g.update(in,2); // 10999 -> enter low-time window
    auto ev=g.takeDeathAudioEvents();
    bool start=false,update=false;
    for(const auto&e:ev){
        start|=e.kind==DeathAudioEventKind::SpatialStart && e.voice==DeathAudioVoiceTag::LowTimeWarning && e.logicalId==0x12u;
        update|=e.kind==DeathAudioEventKind::SpatialUpdate && e.voice==DeathAudioVoiceTag::LowTimeWarning;
    }
    assert(start && update && GameTestProbe::warning(g));

    // Time bonus path: the following gameplay frame must stop the retained voice.
    GameTestProbe::setTimer(g,12000);
    g.update(in,1);
    ev=g.takeDeathAudioEvents();
    bool stop=false;
    for(const auto&e:ev) stop|=e.kind==DeathAudioEventKind::SpatialStop && e.voice==DeathAudioVoiceTag::LowTimeWarning;
    assert(stop && !GameTestProbe::warning(g));

    // 0x4197D8: stack-local accumulator uses strict >1000 comparison.
    GameTestProbe::setTimer(g,50000);
    GameTestProbe::setScore(g,125);
    GameTestProbe::setDecayAccum(g,0);
    GameTestProbe::pressure(g,1000);
    assert(g.score()==125);
    GameTestProbe::pressure(g,1);
    assert(g.score()==75);
    assert(GameTestProbe::decayAccum(g)==1);
    GameTestProbe::pressure(g,1000);
    assert(g.score()==25);
    GameTestProbe::pressure(g,1000);
    assert(g.score()==0); // clamp, never negative

    std::cout << "low-time audio/score r123 ok\n";
}

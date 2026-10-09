#include "game/game.hpp"
#include "game/legacy_interlevel_trace.hpp"
#include "render/legacy_camera.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

struct GameTestProbe { static void beginInterLevel(Game& g,std::size_t next){g.levelIntroScene_.elapsedMs=g.levelIntroScene_.durationMs; g.levelEntryScene_.running=false; g.beginInterLevel(next);} };

static bool near(float a,float b,float e=1e-6f){return std::fabs(a-b)<=e;}

int main(){
    using T=LegacyInterLevel::Trace;
    static_assert(T::durationMs==4000);
    static_assert(T::entryLogicalSfx==0x19u);
    static_assert(T::retainedLogicalSfx==0x07u);
    static_assert(T::speechBySelector[0]==0x33u && T::speechBySelector[1]==0x2Du);

    Game g;
    g.setSpeechEnabled(true);
    const auto oldLevel=g.levelIndex();
    GameTestProbe::beginInterLevel(g,oldLevel+1);
    assert(g.phase()==GamePhase::InterLevel);
    assert(near(g.displayPlayerHeight(),0.008f));
    assert(near(g.displayPlayerRotorRadius(),T::rotorRadius));

    auto ev=g.takeDeathAudioEvents();
    assert(ev.size()==3);
    assert(ev[0].kind==DeathAudioEventKind::SimplePlay && ev[0].logicalId==0x19u);
    assert(ev[1].kind==DeathAudioEventKind::MusicFadeOut && near(ev[1].fadePerMs,T::musicFadePerMs,1e-9f));
    assert(ev[2].kind==DeathAudioEventKind::SpatialStart && ev[2].voice==DeathAudioVoiceTag::InterLevelVoice && ev[2].logicalId==0x07u);

    InputState idle{};
    const float propellerBefore=g.displayPlayerPropellerPhase();
    g.update(idle,1001); // remaining 2999: crosses speech threshold once.
    assert(!near(g.displayPlayerPropellerPhase(),propellerBefore,1e-5f)); // r343 0x41AC97..0x41ACCF
    assert(g.phase()==GamePhase::InterLevel);
    assert(near(g.displayPlayerHeight(),0.008f+1001.f*T::xonixRisePerMs,2e-6f));
    ev=g.takeDeathAudioEvents();
    bool sawUpdate=false,sawSpeech=false;
    for(const auto& e:ev){
        if(e.kind==DeathAudioEventKind::SpatialUpdate && e.voice==DeathAudioVoiceTag::InterLevelVoice)sawUpdate=true;
        if(e.kind==DeathAudioEventKind::SpatialPlay && e.logicalId==0x2Du && near(e.scalar,1.5f))sawSpeech=true;
    }
    assert(sawUpdate && sawSpeech);

    const auto cam=LegacyCamera::interLevelState(g.displayPlayerWorldX(),g.displayPlayerHeight(),g.displayPlayerWorldZ(),0.f,0);
    assert(near(cam.y,0.103f+0.5f*(g.displayPlayerHeight()-0.013000000268220901f),1e-6f));

    g.update(idle,500); // remaining 2499: brightness/fade window active.
    assert(near(g.brightnessScale(),1.f));
    assert(near(g.presentationLightScale(),2499.f/2550.f,2e-4f));
    assert(near(g.legacySfxMasterScale(),2499.f/2550.f,2e-4f));
    g.takeDeathAudioEvents();

    g.update(idle,g.interLevelScene().remainingMs());
    assert(g.phase()==GamePhase::Gameplay);
    assert(g.levelIndex()==oldLevel+1);
    ev=g.takeDeathAudioEvents();
    bool sawStop=false;
    for(const auto& e:ev) if(e.kind==DeathAudioEventKind::SpatialStop && e.voice==DeathAudioVoiceTag::InterLevelVoice)sawStop=true;
    assert(sawStop);
    assert(near(g.brightnessScale(),1.f));
    // r347: loadLevel() enters the next capable intro at RGB 0, so the
    // presentation light is black even though the base brightness is restored.
    assert(g.levelIntroActive());
    assert(near(g.presentationLightScale(),0.f));
    assert(near(g.legacySfxMasterScale(),1.f));
    std::cout<<"interlevel r114 ok\n";
}

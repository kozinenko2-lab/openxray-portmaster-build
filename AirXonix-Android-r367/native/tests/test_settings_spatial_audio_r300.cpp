#include "game/game.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

struct GameTestProbe {
    static void enterSettings(Game& g){ g.enterSettings(); }
    static void startSession(Game& g){ g.startNewSession(0); }
};

static const DeathAudioEvent* findKind(const std::vector<DeathAudioEvent>& ev, DeathAudioEventKind k, DeathAudioVoiceTag v){
    for(const auto& e:ev) if(e.kind==k && e.voice==v) return &e;
    return nullptr;
}

int main(){
    Game g; g.takeDeathAudioEvents();
    GameTestProbe::enterSettings(g);
    assert(g.phase()==GamePhase::Settings);
    assert(g.legacyAudioBasisAngle2()==0);
    auto ev=g.takeDeathAudioEvents();
    auto* start=findKind(ev,DeathAudioEventKind::SpatialStart,DeathAudioVoiceTag::SettingsVoice);
    assert(start && start->logicalId==7);
    assert(std::fabs(start->x)<1e-7f && std::fabs(start->y)<1e-7f && std::fabs(start->z-5.f)<1e-7f);

    for(int i=0;i<20 && !g.settings().readyForInput;++i)g.update({},32);
    g.update({},16);
    ev=g.takeDeathAudioEvents();
    auto* up=findKind(ev,DeathAudioEventKind::SpatialUpdate,DeathAudioVoiceTag::SettingsVoice);
    assert(up && std::fabs(up->z-5.f)<1e-7f);

    // Move to SFX row: the retained voice is pulled from z=5.0 to z=0.1.
    InputState down{}; down.down=true; g.update(down,16); g.takeDeathAudioEvents();
    g.update({},16); ev=g.takeDeathAudioEvents();
    up=findKind(ev,DeathAudioEventKind::SpatialUpdate,DeathAudioVoiceTag::SettingsVoice);
    assert(up && std::fabs(up->z-0.1f)<1e-6f);
    while(std::fabs(g.settings().selectorOffset-LegacySettingsTrace::selectorTarget(g.settings().selected))>1e-7f) g.update({},16);

    // Back exits Settings and stops the retained voice after the original
    // selected-row camera slide has settled.
    InputState back{}; back.back=true; g.update(back,16);
    g.takeDeathAudioEvents();
    for(int i=0;i<30 && g.phase()==GamePhase::Settings;++i)g.update({},16);
    ev=g.takeDeathAudioEvents();
    assert(findKind(ev,DeathAudioEventKind::SpatialStop,DeathAudioVoiceTag::SettingsVoice));
    assert(g.phase()==GamePhase::MainMenu);

    GameTestProbe::startSession(g);
    assert(g.legacyAudioBasisAngle2()==-302);
    std::cout << "settings spatial audio r300 PASS\n";
}

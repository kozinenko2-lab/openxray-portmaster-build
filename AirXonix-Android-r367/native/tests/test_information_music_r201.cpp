#include "game/game.hpp"
#include "game/legacy_controls_trace.hpp"
#include <cassert>
#include <cmath>

struct GameTestProbe {
    static void enterInformation(Game& g){ g.enterInformation(); }
    static void armInformationExit(Game& g){ g.information_.page=2; g.information_.fadeCounter=0x7c0; g.information_.fadeRate=3; g.information_.readyForInput=true; g.information_.inputLatched=false; }
    static void enterControls(Game& g){ g.enterControlsRemap(); }
    static void armControlsCommit(Game& g){ g.controlsRemap_.assigned=4; g.controlsRemap_.awaitingConfirm=true; g.controlsRemap_.fadeCounter=kLegacyControlsTrace.fadeMax; g.controlsRemap_.readyForInput=true; }
};

static bool near(float a,float b,float e=1.0e-9f){ return std::fabs(a-b)<=e; }

int main(){
    Game g;

    // EXE 0x410CF0..0x410D01: fade current M1 music, then queue fixed track 7.
    GameTestProbe::enterInformation(g);
    auto ev=g.takeDeathAudioEvents();
    assert(ev.size()==2);
    assert(ev[0].kind==DeathAudioEventKind::MusicFadeOut);
    assert(near(ev[0].fadePerMs,0.0010000000474974513f));
    assert(ev[1].kind==DeathAudioEventKind::MusicRequest);
    assert(ev[1].logicalId==7u);
    assert(near(ev[1].fadePerMs,0.0010000000474974513f));

    // EXE 0x410D3F..0x410D50: a fresh key starts the -4*dt fade with SFX 0x16.
    // Only after the page counter becomes negative does 0x410CF0 restore M1 music.
    GameTestProbe::armInformationExit(g);
    InputState in{}; in.action=true;
    g.update(in,16);
    assert(g.phase()==GamePhase::Information);
    ev=g.takeDeathAudioEvents();
    assert(ev.size()==1);
    assert(ev[0].kind==DeathAudioEventKind::SimplePlay && ev[0].logicalId==0x16u);

    in={};
    g.update(in,497);
    assert(g.phase()==GamePhase::MainMenu);
    ev=g.takeDeathAudioEvents();
    assert(ev.size()==2);
    assert(ev[0].kind==DeathAudioEventKind::MusicFadeOut);
    assert(near(ev[0].fadePerMs,0.0020000000949949026f));
    assert(ev[1].kind==DeathAudioEventKind::MusicRequest);
    assert(ev[1].logicalId==0u);
    assert(near(ev[1].fadePerMs,0.001500000013038516f));

    // Related direct-EXE audit: Controls fades M1 while open and restores track 0.
    GameTestProbe::enterControls(g);
    ev=g.takeDeathAudioEvents();
    assert(ev.size()==1);
    assert(ev[0].kind==DeathAudioEventKind::MusicFadeOut);
    assert(near(ev[0].fadePerMs,0.0010000000474974513f));
    GameTestProbe::armControlsCommit(g);
    in={}; in.action=true;
    g.update(in,16);
    ev=g.takeDeathAudioEvents();
    assert(ev.size()==1 && ev[0].kind==DeathAudioEventKind::SimplePlay && ev[0].logicalId==0x16u);
    in={}; g.update(in,500);
    ev=g.takeDeathAudioEvents();
    assert(ev.size()==1);
    assert(ev[0].kind==DeathAudioEventKind::MusicRequest);
    assert(ev[0].logicalId==0u);
    assert(near(ev[0].fadePerMs,0.0010000000474974513f));
    return 0;
}

#include <cassert>
#include "game/game.hpp"
#include "game/legacy_controls_trace.hpp"

struct GameTestProbe {
    static void settings(Game& g){ g.enterSettings(); }
    static void controls(Game& g){ g.enterControlsRemap(); }
};

static void tick(Game& g,int ms){ InputState i{}; g.update(i,ms); }
static void arm(Game& g){ while(!g.controlsRemap().readyForInput) tick(g,100); }
static void press(Game& g,int code){ InputState i{}; i.legacyPressedCode=code; g.update(i,16); }
static void finishExit(Game& g){ while(g.phase()==GamePhase::Controls) tick(g,100); }

int main(){
    static_assert(kLegacyControlsTrace.routine==0x00410D60u);
    static_assert(kLegacyControlsTrace.defaults[0]==0x41);
    static_assert(kLegacyControlsTrace.defaults[1]==0x5A);
    static_assert(kLegacyControlsTrace.defaults[2]==0x58);
    static_assert(kLegacyControlsTrace.defaults[3]==0x43);
    static_assert(kLegacyControlsTrace.joystick1(0x100));
    static_assert(kLegacyControlsTrace.joystick2(0x11B));
    assert(!kLegacyControlsTrace.assignable(0x50)); // P reserved for pause
    assert(kLegacyControlsTrace.assignable(0x104));

    Game g; GameTestProbe::settings(g);
    const auto original=g.settings().bindings;
    GameTestProbe::controls(g);
    assert(g.phase()==GamePhase::Controls);
    arm(g);

    press(g,0x50); // forbidden P
    assert(g.controlsRemap().assigned==0);
    press(g,0x44); // D
    assert(g.controlsRemap().assigned==1);
    press(g,0x44); // duplicate, rejected
    assert(g.controlsRemap().assigned==1);
    press(g,0x46); press(g,0x47); press(g,0x104);
    assert(g.controlsRemap().assigned==4);
    assert(g.controlsRemap().awaitingConfirm);
    // Persistent settings still untouched until Enter.
    assert(g.settings().bindings==original);
    press(g,0x0D);
    assert(g.phase()==GamePhase::Controls);
    assert(g.settings().bindings==original);
    finishExit(g);
    assert(g.phase()==GamePhase::Settings);
    assert(g.settings().bindings[0]==0x44);
    assert(g.settings().bindings[1]==0x46);
    assert(g.settings().bindings[2]==0x47);
    assert(g.settings().bindings[3]==0x104);

    // Second pass: Escape cancels all temporary edits transactionally.
    GameTestProbe::controls(g);
    arm(g);
    press(g,0x48);
    InputState esc{}; esc.legacyPressedCode=0x1B; g.update(esc,16);
    assert(g.phase()==GamePhase::Controls);
    finishExit(g);
    assert(g.phase()==GamePhase::Settings);
    assert(g.settings().bindings[0]==0x44);
    return 0;
}

#include "game/game.hpp"
#include "render/legacy_abort_trace.hpp"
#include <cmath>
#include <iostream>
#include <string>

struct GameTestProbe {
    static void beginAbort(Game& g){ g.beginAbortConfirm(); }
    static void updateAbort(Game& g,const InputState& i,int dt){ g.updateAbortConfirm(i,dt); }
    static void forceLeaving(Game& g,float y,bool confirmed){
        g.phase_=GamePhase::Abort;
        g.abortConfirm_.stage=AbortConfirmState::Stage::Leaving;
        g.abortConfirm_.panelY=y;
        g.abortConfirm_.confirmed=confirmed;
        g.abortConfirm_.inputArmed=true;
    }
};

static bool check(bool c,const std::string& s){
    if(c) return true;
    std::cerr << "FAIL: " << s << "\n";
    return false;
}

int main(){
    bool ok=true;
    Game g;
    GameTestProbe::beginAbort(g);
    InputState none{};

    // -0.25 + 269*0.0008 = -0.0348: inside the EXE input window but not yet
    // at the -0.03 hold position. A no-key poll arms the edge-trigger shim.
    GameTestProbe::updateAbort(g,none,269);
    ok &= check(g.phase()==GamePhase::Abort,"abort modal must remain active");
    ok &= check(g.abortConfirm().stage==AbortConfirmState::Stage::Entering,
                "panel must still be Entering at Y about -0.0348");
    ok &= check(g.abortConfirm().panelY>LegacyAbort::Trace::inputThresholdY,
                "panel must have crossed the literal -0.035 input threshold");
    ok &= check(g.abortConfirm().inputArmed,
                "input must arm before the panel reaches the -0.03 hold position");

    InputState yes{}; yes.action=true;
    GameTestProbe::updateAbort(g,yes,0);
    ok &= check(g.abortConfirm().stage==AbortConfirmState::Stage::Leaving,
                "Yes must be accepted while the panel is still in the early input window");
    ok &= check(g.abortConfirm().confirmed,"Yes must set confirmed=true");

    // DIRECT EXE 0x41E615..0x41E626 uses a strict '< -0.25' exit test.
    Game g2;
    GameTestProbe::forceLeaving(g2,LegacyAbort::Trace::offscreenY,false);
    GameTestProbe::updateAbort(g2,none,0);
    ok &= check(g2.phase()==GamePhase::Abort,
                "exactly -0.25 must not finish the Abort modal");
    GameTestProbe::updateAbort(g2,none,1);
    ok &= check(g2.phase()==GamePhase::Gameplay,
                "the modal must finish only after panelY becomes lower than -0.25");

    if(!ok) return 1;
    std::cout << "abort input window r202 ok\n";
    return 0;
}

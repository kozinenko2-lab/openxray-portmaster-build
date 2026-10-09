#include <cassert>
#include <cmath>
#include "game/game.hpp"
#include "game/legacy_settings_trace.hpp"
struct GameTestProbe {
    static void menu(Game& g){g.enterMainMenu();}
    static void start(Game& g){g.startNewSession(0);}
};
int main(){
    static_assert(kLegacySettingsTrace.routine==0x00413670u);
    static_assert(kLegacySettingsTrace.backgroundRoutine==0x00411D60u);
    static_assert(kLegacySettingsTrace.backgroundFrameSubmit==0x00405E40u);
    static_assert(kLegacySettingsTrace.sceneRoutine==0x00412150u);
    static_assert(kLegacySettingsTrace.sceneTextureSlot==3);
    static_assert(kLegacySettingsTrace.backgroundDepthAlways);
    static_assert(kLegacySettingsTrace.backgroundDepthWriteEnabled);
    assert(std::fabs(kLegacySettingsTrace.sessionScale(800.f)-1.f)<1e-7f);
    assert(std::fabs(kLegacySettingsTrace.sessionScale(0.f)-0.8f)<1e-7f);
    assert(std::fabs(kLegacySettingsTrace.sessionScale(1000.f)-1.05f)<1e-7f);
    assert(std::fabs(kLegacySettingsTrace.audioScale(1000.f)-1.f)<1e-7f);
    assert(kLegacySettingsTrace.audioScale(0.f)==0.f);

    Game g; GameTestProbe::menu(g);
    InputState in{}; in.down=true; g.update(in,16); // select Settings
    // r184 selector-slide gate: release frames are consumed until the visual
    // offset reaches row 1 before another M1 event may be accepted.
    in={}; for(int i=0;i<8;++i)g.update(in,16);
    in.action=true; g.update(in,16);
    assert(g.phase()==GamePhase::Settings);
    assert(g.settings().selected==0);
    in={}; for(int i=0;i<20 && !g.settings().readyForInput;++i)g.update(in,32);
    in={}; in.right=true; g.update(in,100);
    assert(g.settings().speed>800.f);
    const float speed=g.settings().speed;
    in={}; in.back=true; g.update(in,16);
    in={}; for(int i=0;i<30 && g.phase()==GamePhase::Settings;++i)g.update(in,16);
    assert(g.phase()==GamePhase::MainMenu);
    GameTestProbe::start(g);
    assert(std::fabs(g.timeScale()-kLegacySettingsTrace.sessionScale(speed))<1e-5f);
}

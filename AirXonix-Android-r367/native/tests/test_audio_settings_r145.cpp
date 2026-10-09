#include <cassert>
#include <cmath>
#include "game/game.hpp"
#include "game/legacy_settings_trace.hpp"
struct GameTestProbe {
    static void setAudio(Game& g,float sfx,float music){g.settings_.sfx=sfx;g.settings_.music=music;}
    static void interLevel(Game& g,float fade){g.phase_=GamePhase::InterLevel;g.interLevelScene_.sfxMasterScale=fade;}
};
static bool near(float a,float b,float e=1e-6f){return std::fabs(a-b)<=e;}
int main(){
    static_assert(kLegacySettingsTrace.sfx==0x025B787Cu);
    static_assert(kLegacySettingsTrace.music==0x025B7880u);
    Game g;
    GameTestProbe::setAudio(g,1000.f,800.f);
    assert(near(g.legacySfxMasterScale(),1.f));
    assert(near(g.legacyMusicMasterScale(),0.85f));
    GameTestProbe::setAudio(g,0.f,0.f);
    assert(g.legacySfxMasterScale()==0.f);
    assert(g.legacyMusicMasterScale()==0.f);
    GameTestProbe::setAudio(g,500.f,500.f);
    const float base=0.25f+500.f*0.00075f;
    assert(near(g.legacySfxMasterScale(),base));
    assert(near(g.legacyMusicMasterScale(),base));
    GameTestProbe::interLevel(g,0.4f);
    assert(near(g.legacySfxMasterScale(),base*0.4f));
    assert(near(g.legacyMusicMasterScale(),base));
    return 0;
}

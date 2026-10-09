#include <cassert>
#include <cmath>
#include "render/legacy_hud.hpp"
#include "game/legacy_settings_trace.hpp"
#include "game/legacy_settings_visual_trace.hpp"
int main(){
    LegacyHudState s; s.screen=LegacyHudScreen::Settings; s.settingsSelected=0;
    s.settingsBrightness={{1.f,.6f,.6f,.6f,.6f,.6f}};
    s.settingsScale={{1.15f,1.f,1.f,1.f,1.f,1.f}};
    s.settingsSpeed=800.f; s.settingsSfx=1000.f; s.settingsMusic=0.f; s.settingsSpeech=true;
    s.settingsSelectorOffset=-.0028f; s.settingsFadeScale=.5f;
    auto q=LegacyHud::compose(s);
    int m2=0,a3=0; bool on=false; for(const auto& v:q){
      if(v.atlas==LegacyHudAtlas::MenuM2){++m2; assert(v.sw==256&&v.sh==42);}
      if(v.atlas==LegacyHudAtlas::Atlas3){++a3; if(v.sx==128&&v.sy==0&&v.sw==64&&v.sh==38)on=true;}
    }
    assert(m2==6); assert(a3==1); assert(on);
    assert(std::fabs(q[0].brightness-.5f)<1e-6f && std::fabs(q[1].brightness-.3f)<1e-6f);
    assert(q[0].w>q[1].w); // selected 1.15x scale
    // r342: Settings labels are submitted at world X=-.006, not centered.
    const float expectedCx=320.f+(-0.006000000052154064f)*320.f/0.018f;
    assert(std::fabs((q[0].x+q[0].w*.5f)-expectedCx)<1e-3f);
    assert((q[0].x+q[0].w*.5f)<320.f);
    const float cameraZ=LegacySettingsTrace::cameraZ(s.settingsSelectorOffset);
    const float expectedCy=240.f+(cameraZ-kLegacySettingsVisualTrace.rowZ(0))*320.f/0.018f;
    assert(std::fabs((q[0].y+q[0].h*.5f)-expectedCy)<1e-3f);
    return 0;
}

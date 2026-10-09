#include "game/game.hpp"
#include "render/legacy_model_factory.hpp"
#include "render/legacy_transform.hpp"
#include "render/legacy_main_menu_decor.hpp"
#include "render/legacy_screen_pipeline.hpp"
#include <cassert>
#include <cmath>
#include <fstream>
#include <iterator>
#include <iostream>
using namespace airxonix;
struct GameTestProbe {
    static void enter(Game& g){g.enterSettings();}
    static void step(Game& g,const InputState& i,int dt){g.updateSettings(i,dt);}
    static SettingsState& state(Game& g){return g.settings_;}
};
static void press(Game& g, bool up){
    GameTestProbe::step(g,InputState{},16);
    InputState a{}; a.up=up; a.down=!up;
    GameTestProbe::step(g,a,16);
}
int main(){
    // Native mod panel is above the original first Settings row.
    Game g;
    GameTestProbe::enter(g);
    InputState neutral{};
    for(int i=0;i<100 && !g.settings().readyForInput;++i) GameTestProbe::step(g,neutral,32);
    assert(g.settings().readyForInput);
    press(g,true); assert(g.settings().selected==7);
    assert(std::fabs(g.settings().selectorOffset)<0.0000001f); // no camera off-screen
    press(g,true); assert(g.settings().selected==6);
    press(g,false); assert(g.settings().selected==7);
    press(g,false); assert(g.settings().selected==0);
    press(g,true); assert(g.settings().selected==7);

    // Landing visuals must not jump from stale cached coordinates to the field.
    GroundEnemy e{};e.x=32.f;e.y=65.f;e.worldX=1.f;e.worldZ=0.f;e.respawnDelay=.5f;
    assert(std::fabs(e.presentationWorldX()-.5f)<.000001f);
    assert(std::fabs(e.presentationWorldZ()-.603125f)<.000001f);
    const auto beforeX=e.presentationWorldX(),beforeZ=e.presentationWorldZ();
    e.respawnDelay=0.f;e.worldX=.5f;e.worldZ=.603125f;
    assert(std::fabs(e.presentationWorldX()-beforeX)<.000001f);
    assert(std::fabs(e.presentationWorldZ()-beforeZ)<.000001f);

    // Check actual projected 3-D mesh geometry instead of changing verified
    // x87 model rotations based on an unaided screenshot impression.
    const auto mesh=LegacyModelFactory::buildMainMenuLogoParts(false).logo;
    const auto proj=LegacyCamera::projectionState(640,480);
    const LegacyScreenPipeline::CameraState cam{0,0,0,0,0,0};
    for(int step=0;step<=10;++step){
        LegacyMainMenuDecorationTrace::TitleState s{};
        s.reveal=step/10.f;s.drop=step==0?.01f:0.f;s.phase=1.f;
        auto m=LegacyTransform::identity();
        LegacyTransform::rotateXRad(m,1.5707963267948966f);
        LegacyTransform::rotateYRad(m,LegacyMainMenuDecorationTrace::logoYaw(s));
        LegacyTransform::setScale(m,LegacyMainMenuDecorationTrace::logoScale(s));
        LegacyScreenPipeline::OutputBatch out{};
        for(const auto& face:mesh.faces){
            std::array<LegacyScreenPipeline::InputVertex,4> vertices{};
            for(size_t i=0;i<face.index.size();++i){
                const auto& v=mesh.vertices[face.index[i]];
                auto p=LegacyTransform::transformPoint(m,v.x,v.y,v.z,
                    LegacyMainMenuDecorationTrace::logoTranslateX(s),
                    LegacyMainMenuDecorationTrace::logoTranslateY(s),
                    LegacyMainMenuDecorationTrace::logoTranslateZ(s));
                vertices[i]={p.x,p.y,p.z,v.u,v.v,1,1,1,1,1};
            }
            LegacyScreenPipeline::appendPolygon(vertices.data(),face.index.size(),cam,proj,out);
        }
        assert(!out.vertices.empty() && !out.indices.empty());
        for(const auto& v:out.vertices){
            assert(std::isfinite(v.screenX)&&std::isfinite(v.screenY));
            assert(v.screenX>=-.001f&&v.screenX<=640.001f);
            assert(v.screenY>=-.001f&&v.screenY<=480.001f);
        }
    }
    std::ifstream f(std::string(AIRXONIX_SOURCE_DIR)+"/portmaster/AirXonix.sh");
    const std::string script((std::istreambuf_iterator<char>(f)),{});
    assert(script.find("VIBRATION_DURATION_MS:-260")!=std::string::npos);
    assert(script.find("VIBRATION_STRENGTH:-82")!=std::string::npos);
    std::cout<<"AirXonix r367 crawler landing, settings, logo projection and rumble PASS\n";
}

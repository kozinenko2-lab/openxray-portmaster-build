#include "render/legacy_main_menu_decor.hpp"
#include "render/legacy_model_factory.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

static bool nearf(float a,float b,float eps=1e-6f){ return std::fabs(a-b)<=eps; }

int main(){
    const auto &t=kLegacyMainMenuDecorationTrace;
    assert(t.titleRoutine==0x00411EF0u);
    assert(t.sceneRoutine==0x00412150u);
    assert(t.modelBuilder==0x004225A0u);
    assert(t.logoMaster==0x02585A60u && t.logoPrepared==0x0257F5C4u);
    assert(t.laxyMaster==0x02585AD8u && t.laxyPrepared==0x025B5B24u);
    assert(t.decorMasterBase==0x02585A74u && t.decorPreparedBase==0x02583718u);
    assert(t.logoTextureSlot==8 && t.laxyTextureSlot==2 && t.decorTextureSlot==3 && t.reflectionTextureSlot==6);
    assert(t.conditionalTexture7Slot==7);
    static_assert(!LegacyMainMenuDecorationTrace::slot5ReferencedByScene);
    static_assert(LegacyMainMenuDecorationTrace::slot6ReferencedByScene);
    static_assert(LegacyMainMenuDecorationTrace::slot6RotateX==-0x200);
    assert(nearf(LegacyMainMenuDecorationTrace::slot6WaveXScale,0.003000000026077032f));
    assert(nearf(LegacyMainMenuDecorationTrace::slot6Y,-0.12300000339746475f));
    assert(nearf(LegacyMainMenuDecorationTrace::slot6Z,-0.28999999165534973f));

    LegacyMainMenuDecorationTrace::TitleState title{};
    float entry=LegacyMainMenuDecorationTrace::entryInitial;
    // 0x411D60 is before 0x411EF0: with reveal still zero, even a full frame
    // leaves the M1 entry phase frozen at -0.5.
    entry=LegacyMainMenuDecorationTrace::advanceEntryPhase(entry,title,100);
    assert(nearf(entry,-0.5f));
    assert(LegacyMainMenuDecorationTrace::introOnly(entry));
    LegacyMainMenuDecorationTrace::advanceTitle(title,1000);
    assert(nearf(title.drop,0.01f));
    assert(nearf(title.phase,1.0f));
    assert(nearf(title.reveal,0.0f));
    LegacyMainMenuDecorationTrace::advanceTitle(title,201);
    assert(nearf(title.drop,0.0f));
    assert(title.waitMs==201);
    assert(nearf(title.phase,1.201f));
    assert(nearf(LegacyMainMenuDecorationTrace::logoTranslateZ(title),0.01f));
    // Force the exact post-hold condition, then verify the one-frame ordering:
    // the frame that first makes reveal non-zero still has entry=-0.5; only
    // the following 0x411D60 advances the menu into view.
    title.drop=0.f; title.waitMs=1501; title.reveal=0.f;
    entry=LegacyMainMenuDecorationTrace::advanceEntryPhase(entry,title,16);
    assert(nearf(entry,-0.5f));
    LegacyMainMenuDecorationTrace::advanceTitle(title,16);
    assert(title.reveal>0.f);
    entry=LegacyMainMenuDecorationTrace::advanceEntryPhase(entry,title,16);
    assert(entry>-0.5f);
    assert(!LegacyMainMenuDecorationTrace::introOnly(entry));

    LegacyMainMenuDecorationTrace::SceneState scene{};
    const auto d=LegacyMainMenuDecorationTrace::advanceScene(scene,1000);
    assert(nearf(scene.a,1.0f));
    assert(nearf(scene.b,1.5f));
    assert(nearf(scene.c,0.2f));
    assert(scene.angle2048==1000);
    assert(scene.timer==1000);
    const float amp=(std::cos(0.2f)+1.f)*0.25f;
    assert(nearf(d.amp,amp));
    assert(nearf(d.yawWave,2.f*amp*std::cos(1.f)));
    assert(nearf(d.pitchWave,amp*std::sin(1.5f)));

    assert(nearf(LegacyMainMenuDecorationTrace::sceneCameraX(),0.f));
    assert(nearf(LegacyMainMenuDecorationTrace::sceneCameraY(),-0.12999999523162842f));
    assert(nearf(LegacyMainMenuDecorationTrace::sceneCameraZ(0.f),-0.30000001192092896f));
    assert(nearf(LegacyMainMenuDecorationTrace::orbitX(0.f),0.14f));
    assert(nearf(LegacyMainMenuDecorationTrace::orbitY(),-0.15f));
    assert(nearf(LegacyMainMenuDecorationTrace::orbitZ(0.f),-0.15f));
    constexpr float halfPi=1.5707963267948966f;
    assert(std::fabs(LegacyMainMenuDecorationTrace::orbitX(halfPi))<1e-6f);
    assert(nearf(LegacyMainMenuDecorationTrace::orbitZ(halfPi),-0.08f));

    const auto titleParts=LegacyModelFactory::buildMainMenuLogoParts(true);
    assert(!titleParts.logo.vertices.empty() && !titleParts.logo.faces.empty());
    assert(!titleParts.laxy.vertices.empty() && !titleParts.laxy.faces.empty());
    for(int i=0;i<7;++i){
        const auto m=LegacyModelFactory::buildMainMenuDecorationSlot(i,true);
        assert(!m.vertices.empty());
        assert(!m.faces.empty());
    }

    std::cout << "r166 direct-EXE main-menu decoration anchors verified\n";
    return 0;
}

#include "render/legacy_gameover_trace.hpp"
#include "render/legacy_screen_pipeline.hpp"
#include "render/legacy_camera.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {
void check(bool ok,const char* msg){
    if(!ok){std::cerr << "FAIL: " << msg << "\n"; std::exit(1);}
}
}

int main(){
    using namespace LegacyScreenPipeline;
    using T=LegacyGameOver::Trace;
    check(T::cameraX==0.f && T::cameraY==0.f && T::cameraZ==0.f,
          "GOVE camera origin must be exactly zero");
    check(T::cameraAngle1==0 && T::cameraAngle2==-510 && T::cameraAngle3==0,
          "GOVE camera angles must be (0,-510,0)");
    check(std::fabs(T::submitZ-0.003000000026077032f)<1e-9f,
          "GOVE submit Z changed");

    // The dedicated GOVE camera must not collapse back to the live death camera.
    const CameraState gove{T::cameraX,T::cameraY,T::cameraZ,
                           T::cameraAngle1,T::cameraAngle2,T::cameraAngle3};
    const auto death=LegacyCamera::deathState(.5015625f,.5f,.4015625f,true,0);
    check(gove.angle2!=death.angle2,"GOVE must not use death camera angle2");

    // Smoke-check that the literal camera produces a finite projected point for
    // the presentation plane. This catches accidental angle/order corruption.
    const auto basis=LegacyCamera::basis(gove.angle1,gove.angle2,gove.angle3);
    const auto v=LegacyCamera::transformRelative(basis,0.f,0.f,T::submitZ);
    check(std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.depth),
          "GOVE camera transform must stay finite");

    std::cout << "gameover dedicated camera r204 ok\n";
    return 0;
}

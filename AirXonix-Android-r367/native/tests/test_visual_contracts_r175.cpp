#include "render/legacy_screen_pipeline.hpp"
#include "render/legacy_camera.hpp"
#include "render/legacy_theme.hpp"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
using namespace LegacyScreenPipeline;
static bool nearf(float a,float b,float e=.02f){return std::fabs(a-b)<=e;}
int main(){
    const auto projection=LegacyCamera::projectionState(640,480);
    // 0x4417D0 / 0x40D1E0: literal XON1 fascia must survive the original
    // front-face test as ONE BL->TL->TR->BR polygon.
    const auto gc=LegacyCamera::gameplayState(.5f,.4f,0.f,0);
    CameraState fieldCamera{gc.x,gc.y,gc.z,gc.angle1,gc.angle2,gc.angle3};
    std::array<InputVertex,4> fascia{{
      {.3875000179f,-.004f,.3840000033f,0.f,.9900000095f,1,1,1,1,1},
      {.3875000179f,.008f,.3875000179f,0.f,.8762500286f,1,1,1,1,1},
      {.6125000119f,.008f,.3875000179f,1.f,.8762500286f,1,1,1,1,1},
      {.6125000119f,-.004f,.3840000033f,1.f,.9900000095f,1,1,1,1,1}}};
    OutputBatch f; assert(appendPolygon(fascia.data(),4,fieldCamera,projection,f)); assert(f.indices.size()==6);

    // r60/r61 M101 final projection. This same world quad now receives the
    // animated camera before phase=0, rather than being frozen as a HUD sprite.
    CameraState menu{0.f,.018f,-.003f,0,-512,0};
    constexpr float hx=.006f,hz=.001f,z=-.0015f;
    std::array<InputVertex,4> q{{
      {-hx,0,z-hz,0,48.f/256.f,1,1,1,1,1}, {-hx,0,z+hz,0,0,1,1,1,1,1},
      { hx,0,z+hz,1,0,1,1,1,1,1}, { hx,0,z-hz,1,48.f/256.f,1,1,1,1,1}}};
    OutputBatch m; assert(appendPolygon(q.data(),4,menu,projection,m));
    assert(nearf(m.vertices[0].screenX,213.3333f)); assert(nearf(m.vertices[1].screenY,195.5556f));
    assert(nearf(m.vertices[2].screenX,426.6667f)); assert(nearf(m.vertices[3].screenY,231.1111f));
    std::puts("visual contracts r175 ok");
}

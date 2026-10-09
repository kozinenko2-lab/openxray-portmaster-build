#include "render/legacy_screen_pipeline.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace LegacyScreenPipeline;

static bool close(float a,float b,float eps=1e-5f){return std::fabs(a-b)<=eps;}

int main(){
    const auto p=LegacyCamera::projectionState(640,480);
    assert(close(p.halfWidth,320.f));
    assert(close(p.halfHeight,240.f));
    assert(close(p.leftSlope,-319.f/320.f));
    assert(close(p.topSlope,-239.f/320.f));

    // Use an identity-like test camera (basis at 0,0,0 includes the recovered
    // Y reflection). Arrange winding so the legacy negative screen-area test
    // accepts the triangle.
    CameraState c{};c.x=c.y=c.z=0.f;c.angle1=c.angle2=c.angle3=0;
    std::array<InputVertex,3> tri{{
        {-0.1f, 0.1f,0.5f,0,0,1,1,0,0,1},
        { 0.1f, 0.1f,0.5f,1,0,1,0,1,0,1},
        { 0.0f,-0.1f,0.5f,.5f,1,1,0,0,1,1}
    }};
    OutputBatch out;
    bool accepted=appendTriangle(tri,c,p,out);
    // If basis reflection changes the screen winding, reverse once and retry;
    // only one orientation must survive the exact sign test.
    if(!accepted){std::swap(tri[1],tri[2]);accepted=appendTriangle(tri,c,p,out);}
    assert(accepted);
    assert(out.indices.size()==3);
    assert(out.vertices.size()==3);
    for(const auto& v:out.vertices){
        assert(close(v.depth,.5f));
        assert(close(v.rhw,2.f));
        // D3D sz=.5 maps to GL NDC z=0 and therefore window depth=.5.
        assert(close(v.clipZ,0.f));
        assert(close(v.clipW,.5f));
    }

    // A triangle crossing the left one-pixel-inset frustum plane must gain an
    // intersection vertex but still emit a valid triangle fan.
    std::array<InputVertex,3> clipped{{
        {-0.7f, 0.0f,0.5f}, // outside: x < -319/320 * depth
        { 0.1f, 0.2f,0.5f},
        { 0.1f,-0.2f,0.5f}
    }};
    OutputBatch clippedOut;
    if(!appendTriangle(clipped,c,p,clippedOut)){
        std::swap(clipped[1],clipped[2]);
        assert(appendTriangle(clipped,c,p,clippedOut));
    }
    assert(clippedOut.vertices.size()==4);
    assert(clippedOut.indices.size()==6);
    for(const auto& v:clippedOut.vertices){
        assert(v.screenX>=1.f-1e-3f); // legacy left plane projects to x=1 pixel
        assert(v.screenX<=639.f+1e-3f);
    }

    // Gameplay camera constants recovered from 0x418D6A..0x418D95.
    CameraState gameplay{};
    assert(close(gameplay.x,.5f));
    assert(close(gameplay.y,.103f));
    assert(close(gameplay.z,.35f));
    assert(gameplay.angle2==-302);

    std::cout<<"legacy_screen_pipeline ok\n";
}

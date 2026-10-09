#include "render/legacy_billboard.hpp"
#include <cassert>
#include <cmath>

int main(){
    using namespace LegacyScreenPipeline;
    CameraState cam{}; cam.x=0.f;cam.y=0.f;cam.z=0.f;cam.angle1=0;cam.angle2=0;cam.angle3=0;
    const auto p=LegacyCamera::projectionState(640,480);
    OutputBatch out;
    LegacyBillboard::Setup setup{.1f,.2f,.05f,.34f,0x00CFAF4Fu};
    // With the zero-angle legacy basis, world +Z maps to positive depth and
    // world +X maps to screen X. Keep point centered and comfortably visible.
    assert(LegacyBillboard::append(0.f,0.f,1.f,cam,p,setup,out));
    assert(out.vertices.size()==4u && out.indices.size()==6u);
    assert(out.indices[0]==3 && out.indices[1]==0 && out.indices[2]==1);
    assert(out.indices[3]==1 && out.indices[4]==2 && out.indices[5]==3);
    assert(std::fabs(out.vertices[0].screenX-320.f)<1e-4f);
    // angle2=0 basis folds Y, so y=0 still lands at half-height.
    assert(std::fabs(out.vertices[0].screenY-240.f)<1e-4f);
    assert(std::fabs(out.vertices[1].screenX-320.34f)<1e-3f);
    assert(std::fabs(out.vertices[2].screenY-240.34f)<1e-3f);
    assert(std::fabs(out.vertices[0].u-.1f)<1e-6f);
    assert(std::fabs(out.vertices[2].u-.15f)<1e-6f);
    // Exact 0x40E120 depth threshold.
    OutputBatch rejected;
    assert(!LegacyBillboard::append(0.f,0.f,.0005f,cam,p,setup,rejected));
    // Exact all-edges-inside guard rejects a partially off-screen quad.
    OutputBatch edge;
    assert(!LegacyBillboard::append(1.f,0.f,1.f,cam,p,setup,edge));
    return 0;
}

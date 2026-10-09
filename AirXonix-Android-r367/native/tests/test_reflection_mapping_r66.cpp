#include "render/legacy_reflection.hpp"
#include <cassert>
#include <cmath>

static bool near(float a,float b,float e=1e-5f){return std::fabs(a-b)<=e;}

int main(){
    // Lock the two helpers to the exact column pairs seen at 0x40E5C0/0x40E630.
    auto m=LegacyTransform::identity();
    LegacyReflection::rotateX(m,3.14159265358979323846f*0.5f);
    assert(near(m.m[0],1.f));
    assert(near(m.m[4],0.f));
    assert(near(m.m[5],1.f));
    assert(near(m.m[7],-1.f));
    assert(near(m.m[8],0.f));

    auto y=LegacyTransform::identity();
    LegacyReflection::rotateY(y,3.14159265358979323846f*0.5f);
    assert(near(y.m[0],0.f));
    assert(near(y.m[2],1.f));
    assert(near(y.m[6],-1.f));
    assert(near(y.m[8],0.f));

    LegacyMesh mesh;
    LegacyMasterVertex v{};
    v.nx=1.f;v.ny=0.f;v.nz=0.f;
    mesh.vertices.push_back(v);
    LegacyReflection::CameraState cam{};
    // Put camera/object in a configuration that produces zero added rotations.
    cam.x=0.f;cam.y=0.f;cam.z=1.f;
    const auto out=LegacyReflection::buildVertices(mesh,LegacyTransform::identity(),0.f,0.f,0.f,cam);
    assert(out.size()==1u);
    // Exact 0x40E8B8..0x40E8D4 sphere-map equations.
    assert(near(out[0].u,1.f));
    assert(near(out[0].v,-0.5f));
    assert(near(out[0].light,1.f));
    assert(near(LegacyReflection::queuedBrightness(0.5f),0.3f));
    return 0;
}

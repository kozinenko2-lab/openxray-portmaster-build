#include "legacy_reflection.hpp"
#include <cmath>

namespace LegacyReflection {

void rotateX(LegacyTransform::Matrix34& m,float a){
    const float c=std::cos(a),s=std::sin(a);
    // 0x40E5C0 mutates offsets (4,8), (16,20), (28,32): Y/Z columns.
    for(int row=0;row<3;++row){
        const int i=row*3;
        const float y=m.m[i+1],z=m.m[i+2];
        m.m[i+1]=y*c-z*s;
        m.m[i+2]=y*s+z*c;
    }
}

void rotateY(LegacyTransform::Matrix34& m,float a){
    const float c=std::cos(a),s=std::sin(a);
    // 0x40E630 mutates offsets (0,8), (12,20), (24,32): X/Z columns.
    for(int row=0;row<3;++row){
        const int i=row*3;
        const float x=m.m[i],z=m.m[i+2];
        m.m[i]=x*c-z*s;
        m.m[i+2]=x*s+z*c;
    }
}

void rotateZ(LegacyTransform::Matrix34& m,float a){
    const float c=std::cos(a),s=std::sin(a);
    // 0x40E6A0 mutates offsets (0,4), (12,16), (24,28): X/Y columns.
    for(int row=0;row<3;++row){
        const int i=row*3;
        const float x=m.m[i],y=m.m[i+1];
        m.m[i]=x*c-y*s;
        m.m[i+1]=x*s+y*c;
    }
}

std::vector<ReflectedVertex> buildVertices(const LegacyMesh& mesh,
                                           const LegacyTransform::Matrix34& model,
                                           float tx,float ty,float tz,
                                           const CameraState& camera){
    LegacyTransform::Matrix34 env=model;

    // 0x40E74B..0x40E7DD: derive two orientation angles from object position
    // relative to the current camera, then rotate the copied model matrix.
    // x87 fpatan is atan2(st1,st0); the explicit pi correction is the legacy
    // quadrant fix around the Z delta.
    const float dx=tx-camera.x;
    const float dy=camera.y-ty;
    const float dz=camera.z-tz;
    const float safeDy=(std::fabs(dy)<1.0e-12f)?std::copysign(1.0e-12f,dy==0.f?1.f:dy):dy;
    const float safeDz=(std::fabs(dz)<1.0e-12f)?std::copysign(1.0e-12f,dz==0.f?1.f:dz):dz;
    const float horizontal=std::atan(dx/safeDy);
    float vertical=std::atan(dy/safeDz);
    if(dz<0.f) vertical+=3.14159265358979323846f;
    rotateX(env,camera.pitchRadians+vertical);
    rotateY(env,camera.yawRadians-horizontal);

    std::vector<ReflectedVertex> out;
    out.reserve(mesh.vertices.size());
    for(const auto& v:mesh.vertices){
        ReflectedVertex r;
        const auto p=LegacyTransform::transformPoint(model,v.x,v.y,v.z,tx,ty,tz);
        r.x=p.x; r.y=p.y; r.z=p.z;

        // 0x40E884..0x40E8D4: only the first two components of the rotated
        // normal are needed. UV is generated directly, not read from the
        // model's authored UVs: U=(1+Nx)/2, V=-(1+Ny)/2.
        const float nx=v.nx*env.m[0]+v.ny*env.m[3]+v.nz*env.m[6];
        const float ny=v.nx*env.m[1]+v.ny*env.m[4]+v.nz*env.m[7];
        r.u=(1.f+nx)*0.5f;
        r.v=-(1.f+ny)*0.5f;
        r.light=1.f;
        out.push_back(r);
    }
    return out;
}

} // namespace LegacyReflection

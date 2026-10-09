#include "legacy_mesh_builder.hpp"
#include <array>
#include <algorithm>
#include <cmath>
#include <utility>

void LegacyMeshBuilder::clear() { mesh_ = {}; }
LegacyMesh LegacyMeshBuilder::take() { return std::exchange(mesh_, {}); }

std::uint32_t LegacyMeshBuilder::addVertex(const LegacyMasterVertex& v) {
    mesh_.vertices.push_back(v);
    return static_cast<std::uint32_t>(mesh_.vertices.size()-1);
}

void LegacyMeshBuilder::addTriangle(std::uint32_t a, std::uint32_t b, std::uint32_t c) {
    mesh_.faces.push_back({{a,b,c}});
}
void LegacyMeshBuilder::addQuad(std::uint32_t a, std::uint32_t b, std::uint32_t c, std::uint32_t d) {
    mesh_.faces.push_back({{a,b,c,d}});
}

void LegacyMeshBuilder::transformRange(std::size_t first, std::size_t count, const LegacyTransform::Matrix34& t) {
    if (first >= mesh_.vertices.size()) return;
    const std::size_t end = std::min(mesh_.vertices.size(), first + count);
    for (std::size_t i=first;i<end;++i) {
        auto& v = mesh_.vertices[i];
        const float x=v.x, y=v.y, z=v.z;
        const float nx=v.nx, ny=v.ny, nz=v.nz;
        v.x = (x*t.m[0] + y*t.m[3] + z*t.m[6]) * t.scalar;
        v.y = (x*t.m[1] + y*t.m[4] + z*t.m[7]) * t.scalar;
        v.z = (x*t.m[2] + y*t.m[5] + z*t.m[8]) * t.scalar;
        v.nx = nx*t.m[0] + ny*t.m[3] + nz*t.m[6];
        v.ny = nx*t.m[1] + ny*t.m[4] + nz*t.m[7];
        v.nz = nx*t.m[2] + ny*t.m[5] + nz*t.m[8];
    }
}


void LegacyMeshBuilder::appendMesh(const LegacyMesh& source,float tx,float ty,float tz) {
    const std::uint32_t base=static_cast<std::uint32_t>(mesh_.vertices.size());
    mesh_.vertices.reserve(mesh_.vertices.size()+source.vertices.size());
    for (auto v:source.vertices) {
        v.x+=tx; v.y+=ty; v.z+=tz;
        mesh_.vertices.push_back(v);
    }
    mesh_.faces.reserve(mesh_.faces.size()+source.faces.size());
    for (const auto& f:source.faces) {
        LegacyFace out;
        out.index.reserve(f.index.size());
        for (const auto i:f.index) out.index.push_back(base+i);
        mesh_.faces.push_back(std::move(out));
    }
}

void LegacyMeshBuilder::appendMeshTransformed(const LegacyMesh& source,
                                              const LegacyTransform::Matrix34& t,
                                              float tx,float ty,float tz) {
    const std::uint32_t base=static_cast<std::uint32_t>(mesh_.vertices.size());
    mesh_.vertices.reserve(mesh_.vertices.size()+source.vertices.size());
    for (auto v:source.vertices) {
        const float x=v.x,y=v.y,z=v.z;
        const float nx=v.nx,ny=v.ny,nz=v.nz;
        v.x=(x*t.m[0]+y*t.m[3]+z*t.m[6])*t.scalar+tx;
        v.y=(x*t.m[1]+y*t.m[4]+z*t.m[7])*t.scalar+ty;
        v.z=(x*t.m[2]+y*t.m[5]+z*t.m[8])*t.scalar+tz;
        v.nx=nx*t.m[0]+ny*t.m[3]+nz*t.m[6];
        v.ny=nx*t.m[1]+ny*t.m[4]+nz*t.m[7];
        v.nz=nx*t.m[2]+ny*t.m[5]+nz*t.m[8];
        mesh_.vertices.push_back(v);
    }
    mesh_.faces.reserve(mesh_.faces.size()+source.faces.size());
    for (const auto& f:source.faces) {
        LegacyFace out;
        out.index.reserve(f.index.size());
        for (const auto i:f.index) out.index.push_back(base+i);
        mesh_.faces.push_back(std::move(out));
    }
}

void LegacyMeshBuilder::addAnnularStrip(float cx,float cz,float innerR,float outerR,float y,
                                        int segments,float start,float sweep,
                                        float u0,float u1,float v0,float v1,bool mirror) {
    if (segments < 1) return;
    const std::size_t base = mesh_.vertices.size();
    const float sign = mirror ? -1.0f : 1.0f;
    for (int i=0;i<=segments;++i) {
        const float f = static_cast<float>(i)/static_cast<float>(segments);
        const float a = start + sign*sweep*f;
        const float c = std::cos(a), s = std::sin(a);
        // 0x403400/0x4036E0 store radial x/z normals with opposite handedness.
        const float nx = c;
        const float nz = sign*s;
        const float u = u0 + (u1-u0)*f;
        LegacyMasterVertex in{};
        in.x=cx + c*innerR; in.y=y; in.z=cz + sign*s*innerR;
        in.nx=nx; in.ny=0.f; in.nz=nz; in.u=u; in.v=v0;
        LegacyMasterVertex out=in;
        out.x=cx + c*outerR; out.z=cz + sign*s*outerR; out.v=v1;
        addVertex(in); addVertex(out);
    }
    for (int i=0;i<segments;++i) {
        const auto a=static_cast<std::uint32_t>(base+i*2);
        const auto b=a+1, c=a+3, d=a+2;
        if (!mirror) addQuad(a,b,c,d); else addQuad(d,c,b,a);
    }
}

// r194 literal transcription of the four circular-cap helpers
//   0x4032A0 (flat, up)   0x403580 (flat, down)
//   0x403400 (bevel, up)  0x4036E0 (bevel, down)
// Vertex i (i=0..N-1) sits at angle a=-2*pi*i/N for the upward helpers and
// a=+2*pi*i/N for the downward ones:
//   pos = (cx+cos a*r, y, cz+sin a*r)
//   nrm = (cos a*cos t, +/-sin t, sin a*cos t)   (flat helpers: (0,+/-1,0))
//   uv  = (uc -/+ cos a*ur, vc + sin a*ur)       (upward uses minus)
// Faces are the literal quad fan (v0, v0+1+2k, v0+2+2k, v0+3+2k) for
// k < (N-4)*0.5+1 (0x4034DB..0x403561). There is no centre vertex.
// The former native centre fan (c,i,i+1) with increasing angle had the
// opposite winding to 0x402A50's lathe, so the CPU facing test culled the
// visible cap and exposed the far cap through the coin rim.
void LegacyMeshBuilder::addLegacyDiscCap(float cx,float y,float cz,float radius,int segments,
                                          float texCenterU,float texCenterV,float texRadius,
                                          bool beveled,float normalTilt,bool downward) {
    if (segments < 3) return;
    constexpr float kTwoPi=6.28318530717958647692f;
    const float radialNormal=beveled ? std::cos(normalTilt) : 0.f;
    const float verticalNormal=(beveled ? std::sin(normalTilt) : 1.f)*(downward ? -1.f : 1.f);
    const auto base=static_cast<std::uint32_t>(mesh_.vertices.size());
    for (int i=0;i<segments;++i) {
        const float a=(downward ? kTwoPi : -kTwoPi)*float(i)/float(segments);
        const float c=std::cos(a), ss=std::sin(a);
        LegacyMasterVertex v{};
        v.x=cx+c*radius; v.y=y; v.z=cz+ss*radius;
        v.nx=c*radialNormal; v.ny=verticalNormal; v.nz=ss*radialNormal;
        v.u=texCenterU + (downward ? c : -c)*texRadius;
        v.v=texCenterV + ss*texRadius;
        addVertex(v);
    }
    const float quadCount=float(segments-4)*0.5f+1.f;
    for (int k=0; float(k)<quadCount; ++k) {
        const int i1=1+2*k, i2=2+2*k, i3=3+2*k;
        if (i2>=segments) break;
        if (i3>=segments) { addTriangle(base,base+std::uint32_t(i1),base+std::uint32_t(i2)); break; }
        addQuad(base,base+std::uint32_t(i1),base+std::uint32_t(i2),base+std::uint32_t(i3));
    }
}

void LegacyMeshBuilder::addCircularCap(float cx,float y,float cz,float radius,int segments,
                                         float texCenterU,float texCenterV,float texRadius,
                                         bool downward) {
    addLegacyDiscCap(cx,y,cz,radius,segments,texCenterU,texCenterV,texRadius,false,0.f,downward);
}

void LegacyMeshBuilder::addBeveledCircularCap(float cx,float y,float cz,float radius,int segments,
                                                    float texCenterU,float texCenterV,float texRadius,
                                                    float normalTilt,bool downward) {
    addLegacyDiscCap(cx,y,cz,radius,segments,texCenterU,texCenterV,texRadius,true,normalTilt,downward);
}

void LegacyMeshBuilder::addAdaptiveSphere(int segments, float radius,
                                          float u0, float u1, float v0, float v1) {
    if (segments < 4) return;
    segments -= segments % 4;
    if (segments < 4) return;

    constexpr float kPi = 3.14159265358979323846f;
    constexpr float kTwoPi = 6.28318530717958647692f;

    // r74 literal 0x401750 transcription. Preserve the dead workspace vertex
    // and exact ring ordering: equator, all upper rings, then all lower rings.
    LegacyMasterVertex scratch{};
    scratch.nx=1.f; scratch.ny=0.f; scratch.nz=0.f;
    scratch.u=0.f; scratch.v=0.f;
    addVertex(scratch);

    struct Ring { std::vector<std::uint32_t> idx; int segments=0; };
    auto emitRing = [&](float latitude, int longitudeSegments) -> Ring {
        Ring ring{}; ring.segments=longitudeSegments;
        const int samples = longitudeSegments > 0 ? longitudeSegments + 1 : 1;
        ring.idx.reserve(static_cast<std::size_t>(samples));
        const float radial=std::cos(latitude), ny=std::sin(latitude);
        const float vv=v0+(v1-v0)*((latitude+kPi*0.5f)/kPi);
        for(int i=0;i<samples;++i){
            const float f=longitudeSegments>0 ? float(i)/float(longitudeSegments) : 0.f;
            const float lon=longitudeSegments>0 ? kTwoPi*f : 0.f;
            const float cx=std::cos(lon),cz=std::sin(lon);
            LegacyMasterVertex v{};
            v.x=radius*radial*cx; v.y=radius*ny; v.z=radius*radial*cz;
            v.nx=radial*cx; v.ny=ny; v.nz=radial*cz;
            v.u=u0+(u1-u0)*f; v.v=vv;
            ring.idx.push_back(addVertex(v));
        }
        return ring;
    };

    Ring equator=emitRing(0.f,segments);
    const int h=segments/4;
    std::vector<Ring> upper; upper.reserve(static_cast<std::size_t>(h));
    std::vector<Ring> lower; lower.reserve(static_cast<std::size_t>(h));
    for(int band=1;band<=h;++band){
        const int m=segments-4*band;
        upper.push_back(emitRing(kTwoPi*float(band)/float(segments),m));
    }
    for(int band=1;band<=h;++band){
        const int m=segments-4*band;
        lower.push_back(emitRing(-kTwoPi*float(band)/float(segments),m));
    }

    if(!upper.empty()&&!lower.empty()){
        const Ring& u=upper.front(); const Ring& l=lower.front();
        for(int q=0;q<4;++q){
            const int S=q*h,T=q*(h-1);
            for(int j=0;j<h-1;++j){
                addTriangle(equator.idx[S+j],u.idx[T+j],equator.idx[S+j+1]);
                addTriangle(u.idx[T+j],u.idx[T+j+1],equator.idx[S+j+1]);
                addTriangle(l.idx[T+j],equator.idx[S+j],equator.idx[S+j+1]);
                addTriangle(l.idx[T+j],equator.idx[S+j+1],l.idx[T+j+1]);
            }
            addTriangle(equator.idx[S+h-1],u.idx[T+h-1],equator.idx[S+h]);
            addTriangle(equator.idx[S+h-1],equator.idx[S+h],l.idx[T+h-1]);
        }
    }

    auto stitchInner=[&](const Ring& source,const Ring& target,bool lowerHemisphere){
        const int qs=source.segments/4,qt=qs-1;
        for(int q=0;q<4;++q){
            const int S=q*qs,T=q*qt;
            for(int j=0;j<qt;++j){
                if(!lowerHemisphere){
                    addTriangle(source.idx[S+j],target.idx[T+j],source.idx[S+j+1]);
                    addTriangle(target.idx[T+j],target.idx[T+j+1],source.idx[S+j+1]);
                }else{
                    addTriangle(source.idx[S+j],source.idx[S+j+1],target.idx[T+j]);
                    addTriangle(target.idx[T+j],source.idx[S+j+1],target.idx[T+j+1]);
                }
            }
            if(!lowerHemisphere) addTriangle(source.idx[S+qt],target.idx[T+qt],source.idx[S+qt+1]);
            else addTriangle(source.idx[S+qt],source.idx[S+qt+1],target.idx[T+qt]);
        }
    };
    for(std::size_t i=1;i<upper.size();++i)stitchInner(upper[i-1],upper[i],false);
    for(std::size_t i=1;i<lower.size();++i)stitchInner(lower[i-1],lower[i],true);
}

void LegacyMeshBuilder::addAdaptiveHemisphere(int segments, float radius,
                                               float u0, float u1, float v0, float v1) {
    if (segments < 4) return;
    segments -= segments % 4;
    if (segments < 4) return;

    constexpr float kPiLocal = 3.14159265358979323846f;
    constexpr float kTwoPiLocal = 6.28318530717958647692f;

    // r257 direct 0x4020D0 transcription. The helper keeps the same dead
    // scratch vertex convention as 0x401750 and emits triangle records only.
    LegacyMasterVertex scratch{};
    scratch.nx=1.f; scratch.ny=0.f; scratch.nz=0.f;
    scratch.u=0.f; scratch.v=0.f;
    addVertex(scratch);

    struct Ring { std::vector<std::uint32_t> idx; int segments=0; };
    auto emitRing = [&](float latitude, int longitudeSegments) -> Ring {
        Ring ring{}; ring.segments=longitudeSegments;
        const int samples=longitudeSegments>0 ? longitudeSegments+1 : 1;
        ring.idx.reserve(static_cast<std::size_t>(samples));
        const float radial=std::cos(latitude), ny=std::sin(latitude);
        const float vf=std::clamp(latitude/(0.5f*kPiLocal),0.f,1.f);
        const float vv=v1+(v0-v1)*vf;
        for(int i=0;i<samples;++i){
            const float f=longitudeSegments>0 ? float(i)/float(longitudeSegments) : 0.f;
            const float lon=longitudeSegments>0 ? kTwoPiLocal*f : 0.f;
            const float cx=std::cos(lon),cz=std::sin(lon);
            LegacyMasterVertex v{};
            v.x=radius*radial*cx; v.y=radius*ny; v.z=radius*radial*cz;
            v.nx=radial*cx; v.ny=ny; v.nz=radial*cz;
            v.u=u0+(u1-u0)*f; v.v=vv;
            ring.idx.push_back(addVertex(v));
        }
        return ring;
    };

    Ring equator=emitRing(0.f,segments);
    const int h=segments/4;
    std::vector<Ring> upper; upper.reserve(static_cast<std::size_t>(h));
    for(int band=1;band<=h;++band){
        const int m=segments-4*band;
        upper.push_back(emitRing(kTwoPiLocal*float(band)/float(segments),m));
    }

    if(!upper.empty()){
        const Ring& u=upper.front();
        for(int q=0;q<4;++q){
            const int S=q*h,T=q*(h-1);
            for(int j=0;j<h-1;++j){
                addTriangle(equator.idx[S+j],u.idx[T+j],equator.idx[S+j+1]);
                addTriangle(u.idx[T+j],u.idx[T+j+1],equator.idx[S+j+1]);
            }
            addTriangle(equator.idx[S+h-1],u.idx[T+h-1],equator.idx[S+h]);
        }
    }

    auto stitchInner=[&](const Ring& source,const Ring& target){
        const int qs=source.segments/4,qt=qs-1;
        for(int q=0;q<4;++q){
            const int S=q*qs,T=q*qt;
            for(int j=0;j<qt;++j){
                addTriangle(source.idx[S+j],target.idx[T+j],source.idx[S+j+1]);
                addTriangle(target.idx[T+j],target.idx[T+j+1],source.idx[S+j+1]);
            }
            addTriangle(source.idx[S+qt],target.idx[T+qt],source.idx[S+qt+1]);
        }
    };
    for(std::size_t i=1;i<upper.size();++i)stitchInner(upper[i-1],upper[i]);
}

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 6.28318530717958647692f;

struct ProfileSample {
    float r=0.f, y=0.f;
    float nr=1.f, ny=0.f;
    float arc=0.f;
};

float safeLength(float x,float y) {
    return std::sqrt(x*x+y*y);
}

void normalize2(float& x,float& y) {
    const float len=safeLength(x,y);
    if (len>1e-12f) { x/=len; y/=len; }
    else { x=1.f; y=0.f; }
}
}

void LegacyMeshBuilder::addRevolvedProfile(const std::vector<LegacyProfilePoint>& profile,
                                           float firstNormalAngle,
                                           float lastNormalAngle,
                                           int radialSegments,
                                           float u0,float u1,float v0,float v1,
                                           float scale) {
    addRevolvedProfileInternal(0.f,kTwoPi,profile,firstNormalAngle,lastNormalAngle,
                               radialSegments,u0,u1,v0,v1,scale);
}

void LegacyMeshBuilder::addRevolvedProfileArc(float sweepStart,float sweepEnd,
                                              const std::vector<LegacyProfilePoint>& profile,
                                              float firstNormalAngle,
                                              float lastNormalAngle,
                                              int radialSegments,
                                              float u0,float u1,float v0,float v1,
                                              float scale) {
    addRevolvedProfileInternal(sweepStart,sweepEnd,profile,firstNormalAngle,lastNormalAngle,
                               radialSegments,u0,u1,v0,v1,scale);
}

void LegacyMeshBuilder::addRevolvedProfileInternal(float sweepStart,float sweepEnd,
                                                   const std::vector<LegacyProfilePoint>& profile,
                                                   float firstNormalAngle,
                                                   float lastNormalAngle,
                                                   int radialSegments,
                                                   float u0,float u1,float v0,float v1,
                                                   float scale) {
    if (profile.size()<2 || radialSegments<1 || scale==0.f) return;

    std::vector<ProfileSample> p(profile.size());
    for (std::size_t i=0;i<profile.size();++i) {
        p[i].r=profile[i].radius*scale;
        p[i].y=profile[i].y*scale;
        if (i>0) {
            const float dr=p[i].r-p[i-1].r;
            const float dy=p[i].y-p[i-1].y;
            p[i].arc=p[i-1].arc+safeLength(dr,dy);
        }
    }

    // Endpoint normals are explicit parameters in 0x402A50/0x402DD0.
    p.front().nr=std::cos(firstNormalAngle);
    p.front().ny=std::sin(firstNormalAngle);
    p.back().nr=std::cos(lastNormalAngle);
    p.back().ny=std::sin(lastNormalAngle);

    // The x87 loop averages adjacent normalized profile slopes and rotates the
    // result by 90 degrees to obtain the 2D profile normal. This form matches
    // the observed divisions by adjacent segment lengths and final normalize.
    for (std::size_t i=1;i+1<p.size();++i) {
        float pr=p[i].r-p[i-1].r;
        float py=p[i].y-p[i-1].y;
        float nr=p[i+1].r-p[i].r;
        float ny=p[i+1].y-p[i].y;
        normalize2(pr,py);
        normalize2(nr,ny);
        float tr=pr+nr;
        float ty=py+ny;
        normalize2(tr,ty);
        p[i].nr=-ty;
        p[i].ny= tr;
    }

    const float totalArc=p.back().arc;
    const std::size_t base=mesh_.vertices.size();
    const std::size_t ringStride=p.size();
    for (int ring=0;ring<=radialSegments;++ring) {
        const float f=float(ring)/float(radialSegments);
        const float angle=sweepStart+(sweepEnd-sweepStart)*f;
        const float c=std::cos(angle), s=std::sin(angle);
        const float uu=u0+(u1-u0)*f;
        for (const auto& q:p) {
            LegacyMasterVertex v{};
            v.x=c*q.r;
            v.y=q.y;
            v.z=s*q.r;
            v.nx=c*q.nr;
            v.ny=q.ny;
            v.nz=s*q.nr;
            v.u=uu;
            const float af=totalArc>1e-12f ? q.arc/totalArc : 0.f;
            v.v=v0+(v1-v0)*af;
            addVertex(v);
        }
    }

    for (int ring=0;ring<radialSegments;++ring) {
        const std::uint32_t a0=static_cast<std::uint32_t>(base+std::size_t(ring)*ringStride);
        const std::uint32_t b0=static_cast<std::uint32_t>(base+std::size_t(ring+1)*ringStride);
        for (std::size_t j=0;j+1<ringStride;++j) {
            addQuad(a0+static_cast<std::uint32_t>(j),
                    b0+static_cast<std::uint32_t>(j),
                    b0+static_cast<std::uint32_t>(j+1),
                    a0+static_cast<std::uint32_t>(j+1));
        }
    }
}

void LegacyMeshBuilder::addBeveledPlane(float minX,float y,float minZ,
                                        float sizeX,float sizeZ,float bevel,
                                        float u0,float u1,float v0,float v1) {
    if (sizeX<=0.f || sizeZ<=0.f) return;
    bevel=std::clamp(bevel,0.f,0.5f*std::min(sizeX,sizeZ));
    const float x1=minX+sizeX, z1=minZ+sizeZ;
    struct P { float x,z; };
    const P p[8]={
        {minX,        minZ+bevel},
        {minX,        z1-bevel},
        {minX+bevel,  z1},
        {x1-bevel,    z1},
        {x1,           z1-bevel},
        {x1,           minZ+bevel},
        {x1-bevel,     minZ},
        {minX+bevel,   minZ}
    };
    std::uint32_t idx[8]{};
    for (int i=0;i<8;++i) {
        LegacyMasterVertex v{};
        v.x=p[i].x; v.y=y; v.z=p[i].z;
        v.nx=0.f; v.ny=1.f; v.nz=0.f;
        v.u=u0+(p[i].x-minX)*(u1-u0)/sizeX;
        // This reversed V equation is visible in 0x4041A0.
        v.v=v1+(p[i].z-minZ)*(v0-v1)/sizeZ;
        idx[i]=addVertex(v);
    }
    addQuad(idx[7],idx[0],idx[1],idx[2]);
    addQuad(idx[7],idx[2],idx[3],idx[6]);
    addQuad(idx[6],idx[3],idx[4],idx[5]);
}

void LegacyMeshBuilder::addBeveledPrism(float minX,float halfHeight,float minZ,
                                        float sizeX,float sizeZ,float bevel,
                                        float u0,float u1,float v0,float v1) {
    if (sizeX<=0.f || sizeZ<=0.f || halfHeight<0.f) return;
    bevel=std::clamp(bevel,0.f,0.5f*std::min(sizeX,sizeZ));
    const float x1=minX+sizeX, z1=minZ+sizeZ;
    struct P { float x,z; };
    const P p[8]={
        {minX,        minZ+bevel},
        {minX,        z1-bevel},
        {minX+bevel,  z1},
        {x1-bevel,    z1},
        {x1,          z1-bevel},
        {x1,          minZ+bevel},
        {x1-bevel,    minZ},
        {minX+bevel,  minZ}
    };

    auto capUv=[&](const P& q,float& u,float& v){
        u=u0+(q.x-minX)*(u1-u0)/sizeX;
        v=v1+(q.z-minZ)*(v0-v1)/sizeZ;
    };

    // r12 trace: 32 vertices total. Keep four 8-vertex rings: top cap,
    // bottom cap, upper side ring, lower side ring. This preserves separate
    // cap-vs-side normal groups while matching the original fixed count.
    std::uint32_t top[8]{},bot[8]{},sideTop[8]{},sideBot[8]{};
    // Literal 0x404620 workspace layout: cap rings are contiguous, not
    // interleaved. top = v0+0..7, bottom = v0+8..15. This matters because the
    // EXE face stream stores workspace byte offsets derived from these exact
    // ordinal positions.
    for(int i=0;i<8;++i){
        float u,v;capUv(p[i],u,v);
        LegacyMasterVertex t{};t.x=p[i].x;t.y=halfHeight;t.z=p[i].z;t.ny=1.f;t.u=u;t.v=v;
        top[i]=addVertex(t);
    }
    for(int i=0;i<8;++i){
        float u,v;capUv(p[i],u,v);
        LegacyMasterVertex b{};b.x=p[i].x;b.y=-halfHeight;b.z=p[i].z;b.ny=-1.f;b.u=u;b.v=v;
        bot[i]=addVertex(b);
    }

    addQuad(top[7],top[0],top[1],top[2]);
    addQuad(top[7],top[2],top[3],top[6]);
    addQuad(top[6],top[3],top[4],top[5]);
    addQuad(bot[2],bot[1],bot[0],bot[7]);
    addQuad(bot[6],bot[3],bot[2],bot[7]);
    addQuad(bot[5],bot[4],bot[3],bot[6]);

    auto edgeNormal=[&](int i,float& nx,float& nz){
        const int j=(i+1)&7;
        const float dx=p[j].x-p[i].x,dz=p[j].z-p[i].z;
        nx=-dz;nz=dx;const float len=safeLength(nx,nz);if(len>1e-12f){nx/=len;nz/=len;}
    };
    std::array<LegacyMasterVertex,8> sideTopVertex{},sideBotVertex{};
    for(int i=0;i<8;++i){
        float pnx=0.f,pnz=0.f,nnx=0.f,nnz=0.f;
        edgeNormal((i+7)&7,pnx,pnz);edgeNormal(i,nnx,nnz);
        float nx=pnx+nnx,nz=pnz+nnz;const float len=safeLength(nx,nz);
        if(len>1e-12f){nx/=len;nz/=len;}else{nx=nnx;nz=nnz;}
        // r261 direct 0x404620: every side-ring vertex samples the same
        // atlas texel pair (arg7,arg9) == (u0,v0). There is no perimeter UV
        // unwrap in the original workspace.
        LegacyMasterVertex a{};a.x=p[i].x;a.y=halfHeight;a.z=p[i].z;a.nx=nx;a.ny=0.f;a.nz=nz;a.u=u0;a.v=v0;
        LegacyMasterVertex b=a;b.y=-halfHeight;b.u=u0;b.v=v0;
        sideTopVertex[std::size_t(i)]=a;
        sideBotVertex[std::size_t(i)]=b;
    }
    // Literal workspace layout continues with contiguous side rings:
    // upper side = v0+16..23, lower side = v0+24..31.
    for(int i=0;i<8;++i) sideTop[i]=addVertex(sideTopVertex[std::size_t(i)]);
    for(int i=0;i<8;++i) sideBot[i]=addVertex(sideBotVertex[std::size_t(i)]);
    // r69 direct EXE trace of 0x404620: the side-wall quads use the opposite
    // winding from the earlier topology-equivalent reconstruction.  In terms
    // of the four 8-vertex rings this is literally
    //   (sideTop[i+1], sideTop[i], sideBot[i], sideBot[i+1])
    // for edges 0..6, followed by the wrap quad
    //   (sideTop[0], sideTop[7], sideBot[7], sideBot[0]).
    // Preserve this order because legacy back-face culling makes it visible.
    for(int i=0;i<7;++i){
        const int j=i+1;
        addQuad(sideTop[j],sideTop[i],sideBot[i],sideBot[j]);
    }
    addQuad(sideTop[0],sideTop[7],sideBot[7],sideBot[0]);
}


void LegacyMeshBuilder::addExtrudedContour(const std::vector<LegacyProfilePoint>& contour,
                                           float firstNormalAngle,
                                           float lastNormalAngle,
                                           float frontDepth,
                                           float backDepth,
                                           float u0,
                                           float u1,
                                           float scale) {
    if (contour.size() < 4 || scale == 0.f) return;

    // r75 literal 0x402510 transcription. Keep the duplicated seam endpoint and
    // emit exactly four P-sized groups; caps are quad-fans, not triangle fans.
    std::vector<LegacyProfilePoint> p=contour;
    const std::size_t P=p.size();
    for(auto& q:p){q.radius*=scale;q.y*=scale;}
    const float zFront=frontDepth*scale,zBack=backDepth*scale;

    struct N2{float x=0.f,y=0.f;};
    std::vector<N2> normals(P);
    auto norm2=[](float& x,float& y){const float l=std::sqrt(x*x+y*y);if(l>1e-12f){x/=l;y/=l;}};
    normals.front()={std::cos(firstNormalAngle),std::sin(firstNormalAngle)};
    normals.back()={std::cos(lastNormalAngle),std::sin(lastNormalAngle)};
    for(std::size_t i=1;i+1<P;++i){
        float ax=p[i].radius-p[i-1].radius,ay=p[i].y-p[i-1].y;
        float bx=p[i+1].radius-p[i].radius,by=p[i+1].y-p[i].y;
        norm2(ax,ay);norm2(bx,by);float tx=ax+bx,ty=ay+by;norm2(tx,ty);
        normals[i]={-ty,tx};
    }
    if(std::fabs(p.front().radius-p.back().radius)<1e-6f &&
       std::fabs(p.front().y-p.back().y)<1e-6f) normals.back()=normals.front();

    const std::uint32_t B=static_cast<std::uint32_t>(mesh_.vertices.size());
    for(std::size_t i=0;i<P;++i){
        LegacyMasterVertex f{},b{};
        f.x=b.x=p[i].radius;f.y=b.y=p[i].y;f.z=zFront;b.z=zBack;
        f.nx=b.nx=normals[i].x;f.ny=b.ny=normals[i].y;f.nz=b.nz=0.f;
        f.u=b.u=u0;f.v=b.v=u1;addVertex(f);addVertex(b);
    }
    for(std::size_t i=0;i+1<P;++i){const auto a=B+static_cast<std::uint32_t>(2*i);addQuad(a,a+1,a+3,a+2);}

    // DIRECT EXE 0x40283D..0x40294C: the two cap groups are not flat-normal
    // faces.  X/Y receive the normalized contour position multiplied by the
    // literal 0.15 coefficient (0x43B27C).  The front group uses NZ=-0.8
    // (0xBF4CCCCD), while the back group uses NZ=+1.0.  The asymmetry is in
    // the original x86 builder and materially changes specular/directional
    // lighting on the heart and other contour-extruded presentation meshes.
    auto capNormal=[](const LegacyProfilePoint& q,float nz){
        LegacyMasterVertex v{};
        const float len=std::sqrt(q.radius*q.radius+q.y*q.y);
        if(len>1e-12f){v.nx=(q.radius/len)*0.15000000596046448f;v.ny=(q.y/len)*0.15000000596046448f;}
        v.nz=nz;
        return v;
    };
    const std::uint32_t C0=static_cast<std::uint32_t>(mesh_.vertices.size());
    for(const auto& q:p){auto v=capNormal(q,-0.800000011920929f);v.x=q.radius;v.y=q.y;v.z=zFront;v.u=u0;v.v=u1;addVertex(v);}
    const std::uint32_t C1=static_cast<std::uint32_t>(mesh_.vertices.size());
    for(const auto& q:p){auto v=capNormal(q,+1.0f);v.x=q.radius;v.y=q.y;v.z=zBack;v.u=u0;v.v=u1;addVertex(v);}

    const std::size_t K=P/2-1;
    for(std::size_t i=0;i<K;++i){
        const auto j=static_cast<std::uint32_t>(2*i);
        addQuad(C0,C0+j+1,C0+j+2,C0+j+3);
        addQuad(C1+j+3,C1+j+2,C1+j+1,C1);
    }
}


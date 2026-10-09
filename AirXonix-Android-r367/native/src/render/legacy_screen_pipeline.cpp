#include "legacy_screen_pipeline.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace LegacyScreenPipeline {
namespace {
constexpr float kDepthEpsilon=1.0e-8f;

enum class Plane { Left,Right,Top,Bottom };

float distance(Plane plane,const ClipVertex& v,const LegacyCamera::ProjectionState& p){
    switch(plane){
        case Plane::Left:   return v.x-p.leftSlope*v.depth;
        case Plane::Right:  return p.rightSlope*v.depth-v.x;
        case Plane::Top:    return v.y-p.topSlope*v.depth;
        case Plane::Bottom: return p.bottomSlope*v.depth-v.y;
    }
    return -1.f;
}

ClipVertex lerp(const ClipVertex& a,const ClipVertex& b,float t){
    auto f=[&](float x,float y){return x+(y-x)*t;};
    return {f(a.x,b.x),f(a.y,b.y),f(a.depth,b.depth),
            f(a.u,b.u),f(a.v,b.v),f(a.light,b.light),
            f(a.r,b.r),f(a.g,b.g),f(a.b,b.b),f(a.a,b.a)};
}

void clipAgainst(std::vector<ClipVertex>& poly,Plane plane,
                 const LegacyCamera::ProjectionState& projection){
    if(poly.empty())return;
    std::vector<ClipVertex> result;
    result.reserve(poly.size()+2);
    ClipVertex prev=poly.back();
    float prevD=distance(plane,prev,projection);
    bool prevInside=prevD>=0.f;
    for(const ClipVertex& cur:poly){
        const float curD=distance(plane,cur,projection);
        const bool curInside=curD>=0.f;
        if(curInside!=prevInside){
            const float denom=prevD-curD;
            if(std::fabs(denom)>std::numeric_limits<float>::epsilon()){
                // Same algebra as the x87 clipping loops: d0/(d0-d1).
                const float t=prevD/denom;
                result.push_back(lerp(prev,cur,t));
            }
        }
        if(curInside)result.push_back(cur);
        prev=cur;prevD=curD;prevInside=curInside;
    }
    poly.swap(result);
}

OutputVertex project(const ClipVertex& in,const LegacyCamera::ProjectionState& p){
    const float rhw=1.f/in.depth; // 0x53B704 is initialized to 1.0 at 0x40C0B1.
    const float sx=in.x*p.focal*rhw+p.halfWidth;
    const float sy=in.y*p.focal*rhw+p.halfHeight;
    const float ndcX=(sx-p.halfWidth)/p.halfWidth;
    const float ndcY=-(sy-p.halfHeight)/p.halfHeight; // D3D screen Y -> GL NDC Y.
    const float w=in.depth;
    return {ndcX*w,ndcY*w,(2.f*in.depth-1.f)*w,w,
            in.u,in.v,in.light,in.r,in.g,in.b,in.a,
            sx,sy,in.depth,rhw};
}

bool legacyFrontFacing(const OutputVertex& a,const OutputVertex& b,const OutputVertex& c){
    // 0x40CE60..0x40CE9C computes
    // (a.x-b.x)*(c.y-b.y) - (c.x-b.x)*(a.y-b.y)
    // and emits the polygon only when the sign bit is set.
    const float area=(a.screenX-b.screenX)*(c.screenY-b.screenY)
                    -(c.screenX-b.screenX)*(a.screenY-b.screenY);
    return std::signbit(area) && area!=0.f;
}
}

bool appendPolygon(const InputVertex* input,std::size_t count,
                   const CameraState& camera,
                   const LegacyCamera::ProjectionState& projection,
                   OutputBatch& out){
    if(!input||count<3||count>64)return false;
    const auto basis=LegacyCamera::basis(camera.angle1,camera.angle2,camera.angle3);
    std::vector<ClipVertex> poly;
    poly.reserve(count+8);
    for(std::size_t i=0;i<count;++i){
        const auto cp=LegacyCamera::transformRelative(basis,
            input[i].x-camera.x,input[i].y-camera.y,input[i].z-camera.z);
        poly.push_back({cp.x,cp.y,cp.depth,input[i].u,input[i].v,input[i].light,
                        input[i].r,input[i].g,input[i].b,input[i].a});
    }

    // The order mirrors the mask bits consumed at 0x40C571, 0x40C757,
    // 0x40C919 and 0x40CAB7: left/right/top/bottom are all side planes; there
    // is no separate near-plane clipping in this routine.
    clipAgainst(poly,Plane::Left,projection);
    clipAgainst(poly,Plane::Right,projection);
    clipAgainst(poly,Plane::Top,projection);
    clipAgainst(poly,Plane::Bottom,projection);
    if(poly.size()<3)return false;
    for(const auto& v:poly)if(!(v.depth>kDepthEpsilon)||!std::isfinite(v.depth))return false;

    std::vector<OutputVertex> projected;
    projected.reserve(poly.size());
    for(const auto& v:poly)projected.push_back(project(v,projection));
    if(!legacyFrontFacing(projected[0],projected[1],projected[2]))return false;

    if(out.vertices.size()+projected.size()>std::numeric_limits<std::uint16_t>::max())return false;
    const auto base=static_cast<std::uint16_t>(out.vertices.size());
    out.vertices.insert(out.vertices.end(),projected.begin(),projected.end());
    // 0x40CEAB..0x40CED7 is a fan: (0,1,2), (0,2,3), ... .
    for(std::size_t i=1;i+1<projected.size();++i){
        out.indices.push_back(base);
        out.indices.push_back(static_cast<std::uint16_t>(base+i));
        out.indices.push_back(static_cast<std::uint16_t>(base+i+1));
    }
    return true;
}

} // namespace LegacyScreenPipeline

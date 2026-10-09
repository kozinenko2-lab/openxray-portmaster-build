#pragma once
#include "legacy_camera.hpp"
#include "legacy_screen_pipeline.hpp"
#include <cstdint>
#include <limits>

namespace LegacyBillboard {

struct Setup {
    float u0=0.f;
    float v0=0.f;
    float extent=0.f;
    float size=0.f; // 0x40E0D0 argument #4; projected edge is size/depth.
    std::uint32_t diffuse=0xffffffffu;
};

inline void unpackRgb(std::uint32_t c,float& r,float& g,float& b){
    r=float((c>>16)&0xffu)/255.f;
    g=float((c>>8)&0xffu)/255.f;
    b=float(c&0xffu)/255.f;
}

// Literal CPU equivalent of AirXonix.wrp.exe:0x40E120 for one 24-byte
// particle record. Unlike the general 0x40C350 polygon path this routine does
// not build a world-space quad or clip it. It projects the record position,
// treats that screen point as the top-left corner, creates a +X/+Y square of
// edge setup.size/depth, and rejects the complete quad if any edge lies
// outside the legacy width-3 / height-3 guard used by 0x40E040/0x40E120.
inline bool append(float worldX,float worldY,float worldZ,
                   const LegacyScreenPipeline::CameraState& camera,
                   const LegacyCamera::ProjectionState& projection,
                   const Setup& setup,
                   LegacyScreenPipeline::OutputBatch& out){
    const auto basis=LegacyCamera::basis(camera.angle1,camera.angle2,camera.angle3);
    const auto cp=LegacyCamera::transformRelative(
        basis,worldX-camera.x,worldY-camera.y,worldZ-camera.z);
    constexpr float kDepthThreshold=0.0010000000474974513f; // 0x3A83126F
    if(!(cp.depth>=kDepthThreshold)) return false;

    const float rhw=1.f/cp.depth;
    const float x0=cp.x*projection.focal*rhw+projection.halfWidth;
    const float y0=cp.y*projection.focal*rhw+projection.halfHeight;
    const float edge=setup.size*rhw;
    const float x1=x0+edge;
    const float y1=y0+edge;
    const float maxX=projection.halfWidth*2.f-3.f;
    const float maxY=projection.halfHeight*2.f-3.f;
    if(x0<0.f || y0<0.f || x1>maxX || y1>maxY) return false;
    if(out.vertices.size()+4u>std::numeric_limits<std::uint16_t>::max()) return false;

    float r,g,b; unpackRgb(setup.diffuse,r,g,b);
    const auto make=[&](float sx,float sy,float u,float v){
        const float ndcX=(sx-projection.halfWidth)/projection.halfWidth;
        const float ndcY=-(sy-projection.halfHeight)/projection.halfHeight;
        const float w=cp.depth;
        return LegacyScreenPipeline::OutputVertex{
            ndcX*w,ndcY*w,(2.f*cp.depth-1.f)*w,w,
            u,v,1.f,r,g,b,1.f,sx,sy,cp.depth,rhw};
    };
    const auto base=static_cast<std::uint16_t>(out.vertices.size());
    const float u1=setup.u0+setup.extent,v1=setup.v0+setup.extent;
    out.vertices.push_back(make(x0,y0,setup.u0,setup.v0));
    out.vertices.push_back(make(x1,y0,u1,setup.v0));
    out.vertices.push_back(make(x1,y1,u1,v1));
    out.vertices.push_back(make(x0,y1,setup.u0,v1));
    // 0x40E040 static index stream: 3,0,1, 1,2,3 for each quad.
    out.indices.insert(out.indices.end(),{
        static_cast<std::uint16_t>(base+3),base,static_cast<std::uint16_t>(base+1),
        static_cast<std::uint16_t>(base+1),static_cast<std::uint16_t>(base+2),static_cast<std::uint16_t>(base+3)});
    return true;
}

inline Setup fieldDebris(int screenWidth){
    return {.939453125f,.126953125f,.05859375f,
            float(screenWidth)*.0005312500288709998f,0x00CFAF4Fu};
}

inline Setup eraserDebris(int screenWidth){
    // 0x4152A2 / 0x4146E1 -> 0x40E0D0: same atlas cell and diffuse as
    // field debris, but a distinct size multiplier 0.0004687500186.
    return {.939453125f,.126953125f,.05859375f,
            float(screenWidth)*.0004687500186264515f,0x00CFAF4Fu};
}

inline Setup pickupSmash(int screenWidth,std::uint32_t color){
    return {.939453125f,.126953125f,.05859375f,
            float(screenWidth)*.0005312500288709998f,color};
}

inline Setup deathBurst(int screenWidth){
    // 0x417470 -> 0x40E0D0: same atlas rectangle, but a slightly larger
    // projected sprite and fixed pink diffuse 0x00FF8F8F.
    return {.939453125f,.126953125f,.05859375f,
            float(screenWidth)*.000625f,0x00FF8F8Fu};
}

inline Setup deathTriColor(int screenWidth,std::uint32_t color){
    // Fresh r92 disassembly of 0x418290 -> 0x40E0D0/0x40E120. All three
    // 160-record groups use the shared debris atlas rectangle and size scale.
    return {.939453125f,.126953125f,.05859375f,
            float(screenWidth)*.0005312500288709998f,color};
}

} // namespace LegacyBillboard

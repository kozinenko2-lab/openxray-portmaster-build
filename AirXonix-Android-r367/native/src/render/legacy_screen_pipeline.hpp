#pragma once
#include "legacy_camera.hpp"
#include <array>
#include <cstdint>
#include <vector>

namespace LegacyScreenPipeline {

// CPU-side equivalent of the transform/clip/project portion of
// AirXonix.wrp.exe:0x40C350.  The original renderer consumed already-lit
// 24-byte vertices, transformed them to a 44-byte temporary clip vertex,
// clipped polygons against four side planes, projected to D3DTLVERTEX and
// finally emitted a 16-bit triangle list.
struct InputVertex {
    float x=0.f,y=0.f,z=0.f;
    float u=0.f,v=0.f,light=1.f;
    float r=1.f,g=1.f,b=1.f,a=1.f;
};

struct ClipVertex {
    float x=0.f,y=0.f,depth=0.f;
    float u=0.f,v=0.f,light=1.f;
    float r=1.f,g=1.f,b=1.f,a=1.f;
};

// GLES-ready pretransformed vertex.  clipW is the legacy camera depth, so the
// GLES perspective interpolator sees 1/w == original D3DTLVERTEX.rhw.
// clipZ is chosen so GL's [-1,+1] NDC depth maps back to the legacy D3D [0,1]
// screen-space z value exactly after the viewport transform.
struct OutputVertex {
    float clipX=0.f,clipY=0.f,clipZ=0.f,clipW=1.f;
    float u=0.f,v=0.f,light=1.f;
    float r=1.f,g=1.f,b=1.f,a=1.f;
    float screenX=0.f,screenY=0.f,depth=0.f,rhw=0.f;
};

struct OutputBatch {
    std::vector<OutputVertex> vertices;
    std::vector<std::uint16_t> indices;
    void clear(){vertices.clear();indices.clear();}
};

struct CameraState {
    float x=0.5f,y=0.103f,z=0.35f;
    int angle1=0;
    int angle2=-302;
    int angle3=0;
};

// Clips and appends one polygon (normally triangle/quad) using the exact four
// side-plane equations recovered from 0x40C0C0 + 0x40C350.  Returns false if
// the polygon is fully clipped, degenerate or back-facing by the original
// screen-space sign test.
bool appendPolygon(const InputVertex* vertices,std::size_t count,
                   const CameraState& camera,
                   const LegacyCamera::ProjectionState& projection,
                   OutputBatch& out);

inline bool appendTriangle(const std::array<InputVertex,3>& tri,
                           const CameraState& camera,
                           const LegacyCamera::ProjectionState& projection,
                           OutputBatch& out){
    return appendPolygon(tri.data(),tri.size(),camera,projection,out);
}

} // namespace LegacyScreenPipeline

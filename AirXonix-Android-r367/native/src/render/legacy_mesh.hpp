#pragma once
#include <cstdint>
#include <vector>

// Exact high-level construction vertex used by the original procedural mesh
// workspace at 0x447A38.  The 32-byte stride is visible throughout the model
// builders and in 0x401010's serializer.
struct LegacyMasterVertex {
    float x=0.f,y=0.f,z=0.f;
    float nx=0.f,ny=1.f,nz=0.f;
    float u=0.f,v=0.f;
};
static_assert(sizeof(LegacyMasterVertex)==32,"legacy master-vertex stride changed");

// 0x401010 stores, inside each finalized model, a render blob beginning with
// u32 vertexCount followed by vertexCount records with a 24-byte stride.
// 0x40C350 reads x/y/z and copies the last three fields unchanged while doing
// the old CPU transform.  r46 direct 0x40C350 trace proves the trailing fields exactly: +0x0c/+0x10
// are texture U/V and +0x14 is a scalar intensity multiplied into the active
// RGB lighting colour before D3D diffuse packing.
struct LegacyRenderVertex {
    float x=0.f,y=0.f,z=0.f;
    float u=0.f,v=0.f;
    float materialOrLight=0.f;
};
static_assert(sizeof(LegacyRenderVertex)==24,"legacy render-vertex stride changed");

// Final output of the old CPU transform/projection frontend at 0x40C350.
// This is layout-compatible with the Direct3D 7 D3DTLVERTEX consumed by the
// legacy draw path. For fidelity the native GLES renderer intentionally mirrors
// this CPU frontend: gl_Position.w is legacy depth so 1/w reproduces D3D7 rhw.
// The struct is also retained for comparison tests and future frame-diff tooling.
struct LegacyTLVertex {
    float sx=0.f, sy=0.f, sz=0.f, rhw=1.f;
    std::uint32_t diffuse=0xffffffffu;
    std::uint32_t specular=0u;
    float u=0.f, v=0.f;
};
static_assert(sizeof(LegacyTLVertex)==32,"legacy TL-vertex stride changed");

// Original topology stream serialized by 0x401010:
//   3, i0, i1, i2            triangle
//   4, i0, i1, i2, i3        quad
//   ...
//   0                         terminator
// Indices and opcodes are 32-bit integers in the x86 data structure.
struct LegacyFace {
    std::vector<std::uint32_t> index; // exactly 3 or 4 in confirmed data
};

struct LegacyMesh {
    std::vector<LegacyMasterVertex> vertices;
    std::vector<LegacyFace> faces;
};

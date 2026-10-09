#pragma once
#include "legacy_mesh.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

// Exact high-level interpretation of finalized model memory produced by
// AirXonix.wrp.exe:0x401010.
struct LegacyFinalizedLayout {
    std::size_t allocationBytes = 0;
    std::size_t sourceVerticesOffset = 0;
    std::size_t transformedVerticesOffset = 0;
    std::size_t preparedBlockOffset = 0;
    std::size_t topologyOffset = 0;
};

struct LegacyLightingState {
    float ambient = 0.f;
    float dirX = 0.f;
    float dirY = -1.f;
    float dirZ = 0.f;
};

class LegacyRuntimeBlob {
public:
    // 0x401010 allocates 0x40 + 88*vertexCount + 20*faceCount bytes.
    // Header pointers then partition that allocation into two 32-byte vertex
    // arrays, one [u32 count + N*24-byte] prepared block and a topology stream.
    static LegacyFinalizedLayout layout(std::size_t vertexCount, std::size_t faceCount);

    // Exact CPU-lighting stage of 0x401570, applied to the transformed master
    // vertex array. Negative N dot (-L) values are clamped to zero; ambient is
    // then added without an upper clamp in this stage.
    static std::vector<LegacyRenderVertex> prepareVertices(
        const std::vector<LegacyMasterVertex>& transformed,
        const LegacyLightingState& lighting);

    // Serialize only the topology part of the original 0x401010 stream:
    // [3,i0,i1,i2] or [4,i0,i1,i2,i3], followed by 0.
    static std::vector<std::uint32_t> serializeTopology(const LegacyMesh& mesh);

    // Native GLES2 index conversion, now confirmed directly by the post-clip
    // fan emitter at 0x40D0C0..0x40D156. Triangles preserve order; a legacy
    // quad (0,1,2,3) becomes (0,1,2),(0,2,3).
    static std::vector<std::uint32_t> triangulateTopology(const LegacyMesh& mesh);
};

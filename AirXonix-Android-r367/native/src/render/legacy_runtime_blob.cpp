#include "legacy_runtime_blob.hpp"
#include <algorithm>
#include <stdexcept>

LegacyFinalizedLayout LegacyRuntimeBlob::layout(std::size_t n, std::size_t f) {
    LegacyFinalizedLayout out;
    out.allocationBytes = 0x40u + 88u*n + 20u*f;
    out.sourceVerticesOffset = 0x20u;
    out.transformedVerticesOffset = 0x20u + 32u*n;
    out.preparedBlockOffset = 0x20u + 64u*n;
    out.topologyOffset = 0x24u + 88u*n;
    return out;
}

std::vector<LegacyRenderVertex> LegacyRuntimeBlob::prepareVertices(
    const std::vector<LegacyMasterVertex>& transformed,
    const LegacyLightingState& lighting) {
    std::vector<LegacyRenderVertex> out;
    out.reserve(transformed.size());
    for (const auto& v : transformed) {
        // 0x401570 loads -lightDir globally before entering the loop, then
        // multiplies normal xyz by those three values.
        float diffuse = v.nx * (-lighting.dirX)
                      + v.ny * (-lighting.dirY)
                      + v.nz * (-lighting.dirZ);
        if (diffuse < 0.f) diffuse = 0.f;
        out.push_back({v.x,v.y,v.z,v.u,v.v,lighting.ambient + diffuse});
    }
    return out;
}

std::vector<std::uint32_t> LegacyRuntimeBlob::serializeTopology(const LegacyMesh& mesh) {
    std::vector<std::uint32_t> out;
    out.reserve(mesh.faces.size()*5u + 1u);
    for (const auto& face : mesh.faces) {
        if (face.index.size()!=3 && face.index.size()!=4)
            throw std::runtime_error("legacy topology accepts only triangles/quads");
        out.push_back(static_cast<std::uint32_t>(face.index.size()));
        for (auto i : face.index) {
            if (i >= mesh.vertices.size()) throw std::runtime_error("legacy topology index out of range");
            out.push_back(i);
        }
    }
    out.push_back(0u);
    return out;
}

std::vector<std::uint32_t> LegacyRuntimeBlob::triangulateTopology(const LegacyMesh& mesh) {
    std::vector<std::uint32_t> out;
    out.reserve(mesh.faces.size()*6u);
    for (const auto& face : mesh.faces) {
        if (face.index.size()==3) {
            for (auto i : face.index) {
                if (i >= mesh.vertices.size()) throw std::runtime_error("legacy triangle index out of range");
                out.push_back(i);
            }
        } else if (face.index.size()==4) {
            const auto a=face.index[0], b=face.index[1], c=face.index[2], d=face.index[3];
            if (a>=mesh.vertices.size()||b>=mesh.vertices.size()||c>=mesh.vertices.size()||d>=mesh.vertices.size())
                throw std::runtime_error("legacy quad index out of range");
            out.insert(out.end(), {a,b,c,a,c,d});
        } else {
            throw std::runtime_error("legacy topology accepts only triangles/quads");
        }
    }
    return out;
}

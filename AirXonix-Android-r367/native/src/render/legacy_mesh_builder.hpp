#pragma once
#include "legacy_mesh.hpp"
#include "legacy_transform.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

struct LegacyProfilePoint {
    float radius = 0.f;
    float y = 0.f;
};

// Native reconstruction of the original procedural mesh workspace around
// 0x447A38/0x445A3C. This stays CPU-side and GLES-independent: builders first
// reproduce the legacy geometry, then the renderer uploads it.
class LegacyMeshBuilder {
public:
    void clear();
    std::size_t vertexCount() const { return mesh_.vertices.size(); }
    std::size_t faceCount() const { return mesh_.faces.size(); }
    const LegacyMesh& mesh() const { return mesh_; }
    LegacyMesh take();

    std::uint32_t addVertex(const LegacyMasterVertex& v);
    void addTriangle(std::uint32_t a, std::uint32_t b, std::uint32_t c);
    void addQuad(std::uint32_t a, std::uint32_t b, std::uint32_t c, std::uint32_t d);

    // Equivalent high-level operation to 0x401490 on a selected vertex range.
    void transformRange(std::size_t first, std::size_t count, const LegacyTransform::Matrix34& t);

    // Native equivalent of 0x403160: append a finalized submesh into the current
    // workspace and add an xyz translation. An optional transform reproduces
    // the 0x401490 working-copy step used by compound model builders.
    void appendMesh(const LegacyMesh& source,
                    float translateX=0.f,
                    float translateY=0.f,
                    float translateZ=0.f);
    void appendMeshTransformed(const LegacyMesh& source,
                               const LegacyTransform::Matrix34& transform,
                               float translateX=0.f,
                               float translateY=0.f,
                               float translateZ=0.f);

    // r74 literal reconstruction of 0x401750: one dead workspace vertex, an
    // N+1 equator seam ring, N-4/N-8/... adaptive rings toward both poles and
    // an exact four-quadrant all-triangle stream with mirrored lower winding.
    // Visible-geometry reconstruction of 0x4032A0/0x403580 circular caps.
    // The x86 workspace uses paired-ring indexing; GLES uses an explicit fan.
    void addCircularCap(float cx,float y,float cz,float radius,int segments,
                        float texCenterU,float texCenterV,float texRadius,
                        bool downward=false);

    // High-quality 0x403400/0x4036E0 pickup-cap presentation: radial
    // perimeter normals are tilted while the proven radial UV mapping stays
    // unchanged.
    // r194 literal 0x4032A0/0x403580/0x403400/0x4036E0 ring + quad fan.
    void addLegacyDiscCap(float cx,float y,float cz,float radius,int segments,
                          float texCenterU,float texCenterV,float texRadius,
                          bool beveled,float normalTilt,bool downward);

    void addBeveledCircularCap(float cx,float y,float cz,float radius,int segments,
                               float texCenterU,float texCenterV,float texRadius,
                               float normalTilt,bool downward=false);

    void addAdaptiveSphere(int segments,
                           float radius,
                           float u0,
                           float u1,
                           float v0,
                           float v1);

    // 0x4020D0: one-sided adaptive hemisphere. The visible geometry uses the
    // same reduced-longitude rings as 0x401750, but only from the equator to
    // the +Y pole. V maps v1 at the equator to v0 at the pole. Xonix uses two
    // copies of this primitive, with the second rotated by pi around X.
    void addAdaptiveHemisphere(int segments,
                               float radius,
                               float u0,
                               float u1,
                               float v0,
                               float v1);

    // 0x402A50: full 360-degree surface-of-revolution builder over a 2D
    // (radius,y) profile. Endpoint normal angles are explicit in the original;
    // interior normals are reconstructed from the same adjacent-segment slope
    // averaging pattern visible in the x87 code. V follows cumulative profile
    // arc length and U follows the revolution seam.
    void addRevolvedProfile(const std::vector<LegacyProfilePoint>& profile,
                            float firstNormalAngle,
                            float lastNormalAngle,
                            int radialSegments,
                            float u0,
                            float u1,
                            float v0,
                            float v1,
                            float scale = 1.f);

    // 0x402DD0: partial-sweep sibling of 0x402A50. Same profile/normals/UV
    // rules, but angles span [sweepStart,sweepEnd] rather than a full turn.
    void addRevolvedProfileArc(float sweepStart,
                               float sweepEnd,
                               const std::vector<LegacyProfilePoint>& profile,
                               float firstNormalAngle,
                               float lastNormalAngle,
                               int radialSegments,
                               float u0,
                               float u1,
                               float v0,
                               float v1,
                               float scale = 1.f);

    // 0x402510: extrudes a closed 2D contour into a thin 3D solid. The legacy
    // caller passes the contour as repeated start/end points, explicit endpoint
    // normal angles for the 2D side profile, front/back depth coordinates, UV
    // bounds and a global scale. r75 now preserves the literal 4P vertex layout,
    // duplicated seam endpoint, P-1 side quads and reversed front/back quad-fans
    // including the original degenerate closing cap quad.
    void addExtrudedContour(const std::vector<LegacyProfilePoint>& contour,
                            float firstNormalAngle,
                            float lastNormalAngle,
                            float frontDepth,
                            float backDepth,
                            float u0,
                            float u1,
                            float scale = 1.f);

    // Exact visible topology of 0x4041A0: a flat beveled octagonal rectangle,
    // 8 perimeter vertices and 3 quads. This supersedes the earlier incorrect
    // "rectangular frame" interpretation.
    void addBeveledPlane(float minX,
                         float y,
                         float minZ,
                         float sizeX,
                         float sizeZ,
                         float bevel,
                         float u0,
                         float u1,
                         float v0,
                         float v1);

    // Address-backed representation of 0x404620: thin beveled octagonal prism.
    // Direct r69 EXE trace proves the fixed output contract and literal face
    // order: exactly 32 master vertices and 14 quads (3 top + 3 bottom + 8 side).
    // Cap winding differs between +Y and -Y and the side ring has its own exact
    // winding. Only the coordinate-derived side UV seam remains to be traced.
    void addBeveledPrism(float minX,
                         float halfHeight,
                         float minZ,
                         float sizeX,
                         float sizeZ,
                         float bevel,
                         float u0,
                         float u1,
                         float v0,
                         float v1);

    // High-confidence reconstruction of the common annular/ribbon primitive
    // generated by 0x403400 / mirrored 0x4036E0.
    void addAnnularStrip(float centerX,
                         float centerZ,
                         float innerRadius,
                         float outerRadius,
                         float y,
                         int segments,
                         float startAngle,
                         float sweep,
                         float u0,
                         float u1,
                         float v0,
                         float v1,
                         bool mirror=false);

private:
    void addRevolvedProfileInternal(float sweepStart,
                                    float sweepEnd,
                                    const std::vector<LegacyProfilePoint>& profile,
                                    float firstNormalAngle,
                                    float lastNormalAngle,
                                    int radialSegments,
                                    float u0,
                                    float u1,
                                    float v0,
                                    float v1,
                                    float scale);

    LegacyMesh mesh_;
};

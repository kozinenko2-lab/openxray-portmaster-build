#pragma once
#include <array>
#include <cstdint>

// r102 direct-EXE reconstruction of 0x004201E0.
// The legacy routine is the central 64x64 field-render frontend. It receives
// the current presentation camera X/Y/Z and frame delta, derives two projected
// 65-boundary grids (64 cells need 65 edges), then dispatches the 0x41F090..
// 0x41FB30 CPU tessellator family with the exact depth/texture ordering.
namespace LegacyFieldFrontend {

inline constexpr std::uint32_t Routine = 0x004201E0u;
inline constexpr std::uint32_t GridX = 0x02584570u;
inline constexpr std::uint32_t GridZ = 0x02584778u;
inline constexpr std::uint32_t GridScaledX = 0x02584674u;
inline constexpr std::uint32_t GridScaledZ = 0x0258487Cu;
inline constexpr int BoundaryCount = 65;
inline constexpr int CellCount = 64;
inline constexpr float SafeTopY = 0.00800000037997961f;
inline constexpr float OuterDepthOffset = 0.03999999910593033f;
inline constexpr float OuterMin = 0.38750001788139343f;
inline constexpr float OuterMax = 0.612500011920929f;
inline constexpr float FieldMin = 0.4000000059604645f;
inline constexpr float FieldMax = 0.6000000238418579f;
inline constexpr float InvCells = 0.015625f;
inline constexpr float GridToMaterialScale = 20.0f;
inline constexpr float BackgroundY = -0.03999999910593033f;
inline constexpr float BackgroundOuterMaxX = 1.0f;
inline constexpr float BackgroundOuterMaxZ = 1.2000000476837158f;
inline constexpr float BackgroundUvScale = 8.0f;
inline constexpr float BackgroundLightZScale = 0.8333333134651184f;
// 0x424C16..0x424C2A seeds the legacy transform with (+pi/4,-pi/4,.8,.2),
// then 0x41ED90 stores (0.2 - sin(-pi/4)*0.8) * 0.9 at 0x25B5B2C.
// 0x41F270 copies that exact value into every non-SAFE floor vertex light.
inline constexpr float NonSafeLight = 0.689116895198822f;

struct GridState {
    float outerRatio{};
    float fieldRatio{};
    float outerMinX{},outerMaxX{},outerMinZ{},outerMaxZ{};
    std::array<float,BoundaryCount> x{};
    std::array<float,BoundaryCount> z{};
    std::array<float,BoundaryCount> scaledX{};
    std::array<float,BoundaryCount> scaledZ{};
};

inline GridState build(float cameraX,float cameraY,float cameraZ){
    GridState out{};
    // x87 sequence 0x4201E6..0x420202 and 0x42033A.
    const float denominator=cameraY-SafeTopY;
    out.outerRatio=(cameraY+OuterDepthOffset)/denominator;
    out.fieldRatio=cameraY/denominator;

    out.outerMinX=cameraX+(OuterMin-cameraX)*out.outerRatio;
    out.outerMaxX=cameraX+(OuterMax-cameraX)*out.outerRatio;
    out.outerMinZ=cameraZ+(OuterMin-cameraZ)*out.outerRatio;
    out.outerMaxZ=cameraZ+(OuterMax-cameraZ)*out.outerRatio;

    const float minX=cameraX+(FieldMin-cameraX)*out.fieldRatio;
    const float maxX=cameraX+(FieldMax-cameraX)*out.fieldRatio;
    const float minZ=cameraZ+(FieldMin-cameraZ)*out.fieldRatio;
    const float maxZ=cameraZ+(FieldMax-cameraZ)*out.fieldRatio;
    const float stepX=(maxX-minX)*InvCells;
    const float stepZ=(maxZ-minZ)*InvCells;
    const float scaledMinX=(minX-FieldMin)*GridToMaterialScale;
    const float scaledMaxX=(maxX-FieldMin)*GridToMaterialScale;
    const float scaledMinZ=(minZ-FieldMin)*GridToMaterialScale;
    const float scaledMaxZ=(maxZ-FieldMin)*GridToMaterialScale;
    const float scaledStepX=(scaledMaxX-scaledMinX)*InvCells;
    const float scaledStepZ=(scaledMaxZ-scaledMinZ)*InvCells;

    for(int i=0;i<BoundaryCount;++i){
        const float fi=static_cast<float>(i);
        out.x[static_cast<std::size_t>(i)]=minX+stepX*fi;
        out.z[static_cast<std::size_t>(i)]=minZ+stepZ*fi;
        out.scaledX[static_cast<std::size_t>(i)]=scaledMinX+scaledStepX*fi;
        out.scaledZ[static_cast<std::size_t>(i)]=scaledMinZ+scaledStepZ*fi;
    }
    return out;
}

struct BackgroundVertex {
    float x{},y{},z{},u{},v{},light{};
};

inline std::array<BackgroundVertex,8> backgroundRing(const GridState& g,float vPhase=0.f){
    const auto make=[vPhase](float x,float z)->BackgroundVertex{
        return {x,BackgroundY,z,x*BackgroundUvScale,z*BackgroundUvScale+vPhase,
                1.0f-z*BackgroundLightZScale};
    };
    // 0x420082..0x4201CD initializes the first four records and the four
    // quad descriptors; 0x420238..0x420334 rewrites the inner four records.
    // r199: 0x41FD50 writes a common V phase to outer records and 0x42031C
    // adds that same phase to camera-dependent inner V coordinates.
    return {
        make(0.0f,0.0f),
        make(0.0f,BackgroundOuterMaxZ),
        make(BackgroundOuterMaxX,BackgroundOuterMaxZ),
        make(BackgroundOuterMaxX,0.0f),
        make(g.outerMinX,g.outerMinZ),
        make(g.outerMinX,g.outerMaxZ),
        make(g.outerMaxX,g.outerMaxZ),
        make(g.outerMaxX,g.outerMinZ)
    };
}

inline constexpr std::array<std::array<std::uint16_t,4>,4> BackgroundQuads{{
    {{0,1,5,4}}, {{5,1,2,6}}, {{7,6,2,3}}, {{0,4,7,3}}
}};

struct RimVertex {
    float x{},y{},z{},u{},v{},light{};
};

inline constexpr int RimRows = 19;
inline constexpr int RimVertexCount = 76;
inline constexpr int RimQuadCount = 38;
inline constexpr float RimY = SafeTopY;
inline constexpr float RimZ0 = 0.38750001788139343f;
inline constexpr float RimZStep = 0.012500000186264515f;
inline constexpr float RimV0 = 0.75f;
inline constexpr float RimVStep = 0.25f;
inline constexpr float RimLight = 0.800000011920929f;
inline constexpr std::array<float,4> RimX{{
    0.3874000012874603f, 0.4000999927520752f,
    0.5990000367164612f, 0.6126000285148621f
}};
inline constexpr std::array<float,4> RimU{{0.75f,1.0f,5.0f,5.25f}};

inline std::array<RimVertex,RimVertexCount> rimVertices(){
    std::array<RimVertex,RimVertexCount> out{};
    for(int column=0;column<4;++column){
        for(int row=0;row<RimRows;++row){
            const auto i=static_cast<std::size_t>(column*RimRows+row);
            out[i]={RimX[static_cast<std::size_t>(column)],RimY,
                    RimZ0+RimZStep*static_cast<float>(row),
                    RimU[static_cast<std::size_t>(column)],
                    RimV0+RimVStep*static_cast<float>(row),RimLight};
        }
    }
    return out;
}

inline std::array<std::array<std::uint16_t,4>,RimQuadCount> rimQuads(){
    std::array<std::array<std::uint16_t,4>,RimQuadCount> q{};
    int n=0;
    for(int i=0;i<18;++i)
        q[static_cast<std::size_t>(n++)]={{static_cast<std::uint16_t>(i),static_cast<std::uint16_t>(i+1),static_cast<std::uint16_t>(20+i),static_cast<std::uint16_t>(19+i)}};
    for(int i=0;i<18;++i)
        q[static_cast<std::size_t>(n++)]={{static_cast<std::uint16_t>(38+i),static_cast<std::uint16_t>(39+i),static_cast<std::uint16_t>(58+i),static_cast<std::uint16_t>(57+i)}};
    q[static_cast<std::size_t>(n++)]={{19,20,39,38}};
    q[static_cast<std::size_t>(n++)]={{36,37,56,55}};
    return q;
}

struct PassTrace {
    std::uint32_t firstTessellator=0x0041F090u;
    std::uint32_t secondTessellator=0x0041F270u;
    std::uint32_t transitionA=0x0041F980u;
    std::uint32_t transitionB=0x0041FB30u;
    std::uint32_t boundaryA=0x0041F460u;
    std::uint32_t boundaryB=0x0041F660u;
    std::uint32_t boundaryC=0x0041F7F0u;
    std::uint32_t hud=0x004247E0u;
    int textureOrder[6]{2,0,1,0,2,3};
};
inline constexpr PassTrace Passes{};

} // namespace LegacyFieldFrontend

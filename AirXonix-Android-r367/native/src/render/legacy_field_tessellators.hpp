#pragma once
#include <array>
#include <cstdint>
#include <vector>

// r105 direct-EXE transcription of the field tessellator family, including 0x41EE90 and the seven 0x41F090..0x41FB30
// tessellators. This header deliberately keeps the CPU scan contract separate
// from GLES so predicates/ranges can be regression-tested headlessly.
namespace LegacyFieldTessellators {
inline constexpr std::uint32_t SafeRuns=0x0041F090u;
inline constexpr std::uint32_t NonSafeRuns=0x0041F270u;
inline constexpr std::uint32_t VerticalSafeEdges=0x0041F460u;
inline constexpr std::uint32_t HorizontalEdgeA=0x0041F660u;
inline constexpr std::uint32_t HorizontalEdgeB=0x0041F7F0u;
inline constexpr std::uint32_t LowHorizontalEdges=0x0041F980u;
inline constexpr std::uint32_t LowVerticalEdges=0x0041FB30u;
inline constexpr int W=64,H=64;

inline constexpr std::uint32_t CaptureMarkerRuns=0x0041EE90u;
inline constexpr int CaptureMarkerCount=15;
struct MarkerRun { int marker=0,y=0,x0=0,x1=0; float height=0.f; }; // x1 exclusive
inline std::vector<MarkerRun> captureMarkerRuns(const std::array<std::uint8_t,W*H>& f,const std::array<float,16>& timer){
    std::vector<MarkerRun> out;
    // 0x4204F4 iterates ids 1..15. 0x41EE90 scans the 62x62 interior and
    // emits one top polygon for every contiguous row run equal to that exact id.
    for(int m=1;m<=15;++m){
        if(timer[static_cast<std::size_t>(m)]<=0.f) continue;
        for(int y=1;y<=62;++y){
            int x=1;
            while(x<=62){
                while(x<=62 && f[static_cast<std::size_t>(y*W+x)]!=static_cast<std::uint8_t>(m)) ++x;
                if(x>62) break;
                const int x0=x++;
                while(x<=62 && f[static_cast<std::size_t>(y*W+x)]==static_cast<std::uint8_t>(m)) ++x;
                out.push_back({m,y,x0,x,timer[static_cast<std::size_t>(m)]});
            }
        }
    }
    return out;
}
inline constexpr std::uint8_t Safe=0x20;
inline constexpr float SafeY=0.00800000037997961f;
inline constexpr float FloorY=0.0f;
inline constexpr float LowEdgeY=0.0005000000237487257f;
inline constexpr float SafeLight=0.800000011920929f;
inline constexpr float EdgeLightA=0.5f;
inline constexpr float EdgeLightB=0.4000000059604645f;
inline constexpr float EdgeLightC=0.6000000238418579f;
// 0x41EE49 initializes the high-wall texture V ceiling to exactly 0.25.
// All three high wall tessellators reuse 0x258498C/0x2584990 as V=0/V=.25.
inline constexpr float HighWallVMin=0.0f;
inline constexpr float HighWallVMax=0.25f;
inline constexpr float LowEdgeLight=0.3499999940395355f;
inline constexpr int LegacyVertexBytes=24;
inline constexpr int ScratchChunkVertices=96;
inline constexpr int ScratchTailThreshold=112;

struct Run { int y=0,x0=0,x1=0; bool safe=false; }; // x1 exclusive
inline std::vector<Run> safeRuns(const std::array<std::uint8_t,W*H>& f){
    std::vector<Run> out;
    for(int y=0;y<H;++y){
        int x=0;
        while(x<W){
            const bool s=f[std::size_t(y*W+x)]==Safe;
            const int begin=x++;
            while(x<W && (f[std::size_t(y*W+x)]==Safe)==s) ++x;
            if(s) out.push_back({y,begin,x,true});
        }
    }
    return out;
}
inline std::vector<Run> nonSafeInteriorRuns(const std::array<std::uint8_t,W*H>& f){
    std::vector<Run> out;
    // 0x41F270 starts at field+65 and scans 62 columns x 62 rows.
    for(int y=1;y<=62;++y){
        int x=1;
        while(x<=62){
            const bool ns=f[std::size_t(y*W+x)]!=Safe;
            const int begin=x++;
            while(x<=62 && (f[std::size_t(y*W+x)]!=Safe)==ns) ++x;
            if(ns) out.push_back({y,begin,x,false});
        }
    }
    return out;
}

enum class EdgeDir : std::uint8_t { SafeToNonSafe, NonSafeToSafe };
struct Edge { int x=0,y=0; EdgeDir dir=EdgeDir::SafeToNonSafe; };
inline std::vector<Edge> verticalTransitions(const std::array<std::uint8_t,W*H>& f){
    std::vector<Edge> out;
    // 0x41F460: field+65; 62 columns, 63 rows; compare current with previous row.
    for(int y=1;y<=63;++y) for(int x=1;x<=62;++x){
        const bool cur=f[std::size_t(y*W+x)]==Safe;
        const bool prev=f[std::size_t((y-1)*W+x)]==Safe;
        if(cur!=prev) out.push_back({x,y,cur?EdgeDir::NonSafeToSafe:EdgeDir::SafeToNonSafe});
    }
    return out;
}
// r107 correction from literal machine flow: the low family is directional,
// not symmetric. 0x41F980 accepts only SAFE -> non-SAFE across +X. 0x41FB30
// accepts only previous-row SAFE -> current-row non-SAFE and merges contiguous
// X cells into one four-vertex band.
inline std::vector<Edge> lowHorizontalTransitions(const std::array<std::uint8_t,W*H>& f){
    std::vector<Edge> out;
    for(int y=1;y<=62;++y) for(int x=0;x<=61;++x){
        if(f[std::size_t(y*W+x)]==Safe && f[std::size_t(y*W+x+1)]!=Safe)
            out.push_back({x,y,EdgeDir::SafeToNonSafe});
    }
    return out;
}
struct LowRun { int y=0,x0=0,x1=0; }; // x1 exclusive; condition holds in [x0,x1)
inline std::vector<LowRun> lowVerticalRuns(const std::array<std::uint8_t,W*H>& f){
    std::vector<LowRun> out;
    for(int y=1;y<=63;++y){
        int x=0;
        while(x<64){
            while(x<64 && !(f[std::size_t((y-1)*W+x)]==Safe && f[std::size_t(y*W+x)]!=Safe)) ++x;
            if(x>=64) break;
            const int x0=x++;
            while(x<64 && f[std::size_t((y-1)*W+x)]==Safe && f[std::size_t(y*W+x)]!=Safe) ++x;
            out.push_back({y,x0,x});
        }
    }
    return out;
}
// Compatibility enumerator for callers/tests that need individual cells.
inline std::vector<Edge> lowVerticalTransitions(const std::array<std::uint8_t,W*H>& f){
    std::vector<Edge> out;
    for(const auto& r:lowVerticalRuns(f))
        for(int x=r.x0;x<r.x1;++x) out.push_back({x,r.y,EdgeDir::SafeToNonSafe});
    return out;
}

inline constexpr float StaticGridMin=0.4000000059604645f; // 0x43B410
inline constexpr float StaticGridStep=0.0031250000465661287f; // 0x43B47C
inline constexpr float StaticUvStep=0.0625f; // 0x43B590
struct LowVertex { float x=0,y=LowEdgeY,z=0,u=0,v=0,light=LowEdgeLight; };
inline constexpr float gridCoord(int i){return StaticGridMin+StaticGridStep*static_cast<float>(i);}
inline constexpr float gridUv(int i){return StaticUvStep*static_cast<float>(i);}

// Literal four-record run polygon used by 0x41EE90 and 0x41F090.  The scratch
// stream writes the left endpoint pair first, then the right endpoint pair in
// reversed Z order: BL/TL/TR/BR in the field X/Z plane.
struct TopVertex { float x=0,y=0,z=0,u=0,v=0,light=1.f; };
inline std::array<TopVertex,4> staticTopRunQuad(int row,int x0,int x1,float height,float light){
    return {{{gridCoord(x0),height,gridCoord(row),  gridUv(x0),gridUv(row),  light},
             {gridCoord(x0),height,gridCoord(row+1),gridUv(x0),gridUv(row+1),light},
             {gridCoord(x1),height,gridCoord(row+1),gridUv(x1),gridUv(row+1),light},
             {gridCoord(x1),height,gridCoord(row),  gridUv(x1),gridUv(row),  light}}};
}
inline std::array<TopVertex,4> safeRunQuad(const Run& r){
    return staticTopRunQuad(r.y,r.x0,r.x1,SafeY,SafeLight);
}

inline LowVertex lowV(int xi,int zi){return {gridCoord(xi),LowEdgeY,gridCoord(zi),gridUv(xi),gridUv(zi),LowEdgeLight};}
// Literal record ordering from 0x41F980. Every accepted transition writes four
// 24-byte vertices; 0x40D1E0 later triangulates the four-record polygon.
inline std::array<LowVertex,4> lowHorizontalQuad(int x,int y){
    return {{lowV(x+2,y+1),lowV(x+1,y),lowV(x+1,y+1),lowV(x+2,y+2)}};
}
// Literal four-record polygon produced by one contiguous run in 0x41FB30.
inline std::array<LowVertex,4> lowVerticalQuad(const LowRun& r){
    return {{lowV(r.x0,r.y),lowV(r.x0+1,r.y+1),lowV(r.x1+1,r.y+1),lowV(r.x1,r.y)}};
}

struct ScanContract { std::uint32_t address; int startOffset; int rows; int columns; int rowSkip; };
inline constexpr std::array<ScanContract,7> Contracts{{
    {SafeRuns,0,64,64,0},
    {NonSafeRuns,65,62,62,2},
    {VerticalSafeEdges,65,63,62,2},
    {HorizontalEdgeA,64,62,48,16},
    {HorizontalEdgeB,79,62,48,16},
    {LowHorizontalEdges,64,62,62,2},
    {LowVerticalEdges,64,63,64,0}
}};
}

namespace LegacyFieldTessellators {
inline std::vector<Edge> highHorizontalTransitions(const std::array<std::uint8_t,W*H>& f){
    std::vector<Edge> out;
    // 0x41F660: field+64, 62 rows, x=0..47, only SAFE -> non-SAFE.
    for(int y=1;y<=62;++y) for(int x=0;x<=47;++x){
        if(f[std::size_t(y*W+x)]==Safe && f[std::size_t(y*W+x+1)]!=Safe)
            out.push_back({x+1,y,EdgeDir::SafeToNonSafe});
    }
    // 0x41F7F0: field+79, 62 rows, x=15..62, only non-SAFE -> SAFE.
    for(int y=1;y<=62;++y) for(int x=15;x<=62;++x){
        if(f[std::size_t(y*W+x)]!=Safe && f[std::size_t(y*W+x+1)]==Safe)
            out.push_back({x+1,y,EdgeDir::NonSafeToSafe});
    }
    return out;
}
inline bool highWallXVisible(int safeX,int y,bool positiveSide){
    if(y<1 || y>62) return false;
    // positive side: SAFE cell x -> non-SAFE x+1, 0x41F660 scans x<=47.
    if(positiveSide) return safeX>=0 && safeX<=47;
    // negative side: non-SAFE x-1 -> SAFE x, 0x41F7F0 scans left cell x-1>=15.
    return safeX>=16 && safeX<=63;
}
inline bool highWallZVisible(int x,int boundaryY){
    // 0x41F460 scans x=1..62 and row boundaries y=1..63.
    return x>=1 && x<=62 && boundaryY>=1 && boundaryY<=63;
}

// r171: keep the three high-wall tessellators as distinct material families.
// 0x41F460 walks row boundaries and emits only non-SAFE -> SAFE transitions;
// the legacy scratch stream represents each consecutive accepted span by
// endpoint vertex pairs, so expose those spans instead of forcing cell quads.
struct HighWallRun { int boundary=0,a0=0,a1=0; EdgeDir dir=EdgeDir::SafeToNonSafe; }; // a1 exclusive
inline std::vector<HighWallRun> highVerticalRuns(const std::array<std::uint8_t,W*H>& f){
    std::vector<HighWallRun> out;
    // r217 direct 0x41F495/0x41F49C: this pass is directional. It starts a run
    // only when the current-row cell is SAFE and the previous-row cell is not.
    // The opposite SAFE->non-SAFE Z face is never emitted by 0x41F460.
    for(int y=1;y<=63;++y){
        int x=1;
        while(x<=62){
            const bool cur=f[std::size_t(y*W+x)]==Safe;
            const bool prev=f[std::size_t((y-1)*W+x)]==Safe;
            if(!(cur && !prev)){ ++x; continue; }
            const int x0=x++;
            while(x<=62){
                const bool c=f[std::size_t(y*W+x)]==Safe;
                const bool p=f[std::size_t((y-1)*W+x)]==Safe;
                if(!(c && !p)) break;
                ++x;
            }
            out.push_back({y,x0,x,EdgeDir::NonSafeToSafe});
        }
    }
    return out;
}

// Lighting constants are owned by the three consecutive high-wall routines,
// not by the SAFE top pass: 0x41F460=.5, 0x41F660=.4, 0x41F7F0=.6.
inline constexpr float highWallLight(std::uint32_t routine){
    return routine==VerticalSafeEdges ? EdgeLightA :
           routine==HorizontalEdgeA  ? EdgeLightB :
           routine==HorizontalEdgeB  ? EdgeLightC : SafeLight;
}

// r218 literal 24-byte vertex streams for the three high-wall tessellators.
// 0x40D1E0 consumes them in groups of four, so preserve each quad until after
// legacy clipping/culling instead of pre-splitting it into triangles.
inline std::array<TopVertex,4> highVerticalRunQuad(const HighWallRun& r){
    const float z=gridCoord(r.boundary),x0=gridCoord(r.a0),x1=gridCoord(r.a1);
    const float u0=gridUv(r.a0),u1=gridUv(r.a1),l=EdgeLightA;
    return {{{x0,0.f,z,u0,HighWallVMin,l},
             {x0,SafeY,z,u0,HighWallVMax,l},
             {x1,SafeY,z,u1,HighWallVMax,l},
             {x1,0.f,z,u1,HighWallVMin,l}}};
}
inline std::array<TopVertex,4> highHorizontalAQuad(const Edge& e){
    const float x=gridCoord(e.x),z0=gridCoord(e.y),z1=gridCoord(e.y+1);
    const float u0=gridUv(e.y),u1=gridUv(e.y+1),l=EdgeLightB;
    return {{{x,0.f,z0,u0,HighWallVMin,l},
             {x,SafeY,z0,u0,HighWallVMax,l},
             {x,SafeY,z1,u1,HighWallVMax,l},
             {x,0.f,z1,u1,HighWallVMin,l}}};
}
inline std::array<TopVertex,4> highHorizontalBQuad(const Edge& e){
    const float x=gridCoord(e.x),z0=gridCoord(e.y),z1=gridCoord(e.y+1);
    const float u0=gridUv(e.y),u1=gridUv(e.y+1),l=EdgeLightC;
    return {{{x,SafeY,z0,u0,HighWallVMax,l},
             {x,0.f,z0,u0,HighWallVMin,l},
             {x,0.f,z1,u1,HighWallVMin,l},
             {x,SafeY,z1,u1,HighWallVMax,l}}};
}
}

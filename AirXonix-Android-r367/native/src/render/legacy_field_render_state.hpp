#pragma once
#include <array>
#include <cstddef>

// Direct EXE ownership around 0x42043F..0x420542 plus r219 audit of 0x405A20.
// 0x405A20 is only SetRenderState(D3DRS_ZFUNC=0x17,...):
//   argument 0 -> D3DCMP_ALWAYS   (8)
//   argument 1 -> D3DCMP_LESSEQUAL(4)
// It does NOT disable Z testing and does NOT change ZWRITEENABLE.  Native GL
// code must therefore represent ALWAYS with glDepthFunc(GL_ALWAYS), not with
// glDisable(GL_DEPTH_TEST), so depth writes remain possible like the D3D path.
namespace LegacyFieldRenderState {
constexpr std::size_t FloorTextureSlot = 0;
constexpr std::size_t BackgroundPreparedTextureSlot = 1;
constexpr std::size_t SafeAndBoundaryTextureSlot = 2;
constexpr std::size_t FrontLogoTextureSlot = 3;
constexpr std::size_t ShadowTextureSlot = 0;

constexpr bool BackgroundUsesZAlways = true;
constexpr bool FloorUsesZAlways = true;       // 0x41F270 is submitted before LEQUAL restore.
constexpr bool BoundariesUseTexture2 = true;
constexpr bool ZAlwaysKeepsDepthTestEnabled = true;
constexpr bool ZAlwaysKeepsDepthWritesEnabled = true;

enum class DepthCompare {
    Always,
    LessEqual
};

enum class FieldPass {
    PreparedRim,       // 0x40C350(0x2583948), texture 2
    SafeTop,           // 0x41F090, texture 2
    CrawlerPass1,      // 0x418840
    NonSafeFloor,      // 0x41F270, texture 0
    CrawlerPass2,      // 0x418A90
    BackgroundRing,    // 0x40C350(0x2583600), texture 1
    LowEdges,          // 0x41F980 + 0x41FB30, texture 0
    HighWalls,         // 0x41F460 + 0x41F660 + 0x41F7F0, texture 2
    CaptureMarkers,    // 0x41EE90 loop, inherits texture 2
    FrontLogo          // 0x40D1E0(0x4417D0), texture 3
};

struct PassDepthState {
    FieldPass pass;
    DepthCompare depth;
};

// Literal caller order from 0x42043F..0x420542.  Repeated state writes are
// intentionally collapsed into the depth state inherited by each submit.
inline constexpr std::array<PassDepthState,10> PassOrder{{
    {FieldPass::PreparedRim,    DepthCompare::Always},
    {FieldPass::SafeTop,        DepthCompare::Always},
    {FieldPass::CrawlerPass1,   DepthCompare::Always},
    {FieldPass::NonSafeFloor,   DepthCompare::Always},
    {FieldPass::CrawlerPass2,   DepthCompare::LessEqual},
    {FieldPass::BackgroundRing, DepthCompare::Always},
    {FieldPass::LowEdges,       DepthCompare::LessEqual},
    {FieldPass::HighWalls,      DepthCompare::LessEqual},
    {FieldPass::CaptureMarkers, DepthCompare::LessEqual},
    {FieldPass::FrontLogo,      DepthCompare::Always}
}};
}

#pragma once
#include <cstdint>

// Direct render-state anchors for the third Information page (0x414170).
// 0x405A20 changes only D3DRENDERSTATE_ZFUNC; it never disables Z or ZWRITE.
namespace LegacyInformationRenderState {
inline constexpr std::uint32_t Routine=0x00414170u;
inline constexpr std::uint32_t SetAlwaysCall=0x00414319u;
inline constexpr std::uint32_t RestoreLessEqualCall=0x004143F2u;
inline constexpr bool FlyingMineDepthTestEnabled=true;
inline constexpr bool FlyingMineDepthWriteEnabled=true;
inline constexpr bool FlyingMineDepthAlways=true;
inline constexpr int TextTextureSlot=5;
inline constexpr bool TextSharesAlwaysDepth=true;
inline constexpr bool Page0TextAdditiveBlend=true;
inline constexpr bool Page1TextAdditiveBlend=true;
inline constexpr bool Page2TextAdditiveBlend=true;
inline constexpr std::uint32_t Page0TextFlushCall=0x00410470u;
inline constexpr std::uint32_t Page1TextFlushCall=0x004106D8u;
inline constexpr std::uint32_t Page2TextFlushCall=0x004143CFu;
inline constexpr std::uint32_t Page1AlwaysAirSubtype2SubmitCall=0x0041075Fu;
inline constexpr std::uint32_t Page1AlwaysAirSubtype0SubmitCall=0x00410791u;
inline constexpr std::uint32_t Page1RestoreLessEqualCall=0x00410798u;
}

#pragma once
#include <array>
#include <cstdint>
#include <string_view>
namespace LegacyCinematicFamily {
struct Slot { int index; std::uint32_t prepared; std::string_view resource; };
inline constexpr std::array<Slot,6> Slots{{
    {0,0x0257F5CCu,"COMP"},{1,0x0257F5D0u,"LEV2"},{2,0x0257F5D4u,"PAUS"},
    {3,0x0257F5D8u,"GOVE"},{4,0x0257F5DCu,"ABOR"},{5,0x0257F5E0u,"GAME"}
}};
}

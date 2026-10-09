#pragma once
#include <array>
#include <string>
#include <cstdint>

namespace LegacyControlsVisual {
struct Trace {
    std::uint32_t routine=0x00410D60u;
    std::uint32_t gridClear=0x0040BE90u;
    std::uint32_t putText=0x0040BF60u;
    std::uint32_t drawGrid=0x0040BF40u;
    int textureSlot=5;
    int columns=40,rows=15,cellW=16,cellH=32;
};
inline constexpr Trace kTrace{};

std::array<char,8> keyName(int legacyCode);
} // namespace LegacyControlsVisual

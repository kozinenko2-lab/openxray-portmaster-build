#pragma once
#include <cstdint>

namespace airxonix {

// DIRECT EXE r147: 0x40AA80 consumes the 32-bit software-mixer accumulator.
// Each accumulator lane starts at 128 (0x409EF4..0x409EFE). 0x40AAAE performs
// an arithmetic >>8 and indexes two 8-byte-stride tables rooted at
// 0x43FCA8/0x43FCAC. The table pairs implement branchless U8 saturation:
// negative high byte -> 0, high byte 0 -> preserve low byte, positive high
// byte -> 255. Therefore this is exactly clamp(accumulator,0,255), and with
// the +128 accumulator bias it is equivalent to clamp(sum,-128,127)+128.
struct LegacyOutputQuantizerTrace {
    static constexpr std::uint32_t routine = 0x0040AA80u;
    static constexpr std::uint32_t maskTable = 0x0043FCA8u;
    static constexpr std::uint32_t addTable = 0x0043FCACu;
    static constexpr int accumulatorBias = 128;

    static constexpr std::uint8_t quantizeAccumulator(int accumulator) {
        return static_cast<std::uint8_t>(accumulator < 0 ? 0 : accumulator > 255 ? 255 : accumulator);
    }
    static constexpr std::uint8_t quantizeCenteredSum(int sum) {
        return quantizeAccumulator(sum + accumulatorBias);
    }
};

} // namespace airxonix

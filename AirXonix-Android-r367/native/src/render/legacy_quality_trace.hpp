#pragma once
#include <cstdint>

// r249 DIRECT EXE audit of the geometry-quality global used by the procedural
// model constructors. 0x407568 sets EBX=1 on the successful renderer-init path;
// 0x40780B is the only write to 0x43F0AC and copies that EBX value there.
// Every later xref only reads the global (0x420660..0x42278D families).
// Therefore the shipped final EXE always builds the high-quality geometry on
// a successful graphics initialization; this is not a user-facing quality
// setting and should not be dynamically toggled by the native port.
struct LegacyQualityTrace {
    static constexpr std::uint32_t globalAddress = 0x0043F0ACu;
    static constexpr std::uint32_t successSetsOne = 0x00407568u;
    static constexpr std::uint32_t onlyWrite = 0x0040780Bu;
    static constexpr int initializedValue = 1;
    static constexpr bool highQuality = initializedValue != 0;
};

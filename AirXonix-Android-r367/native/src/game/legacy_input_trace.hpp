#pragma once
#include <array>
#include <cstdint>

struct LegacyGameplayInputTrace {
    std::uint32_t function=0x00419270u;
    std::uint32_t eventPoll=0x00409410u;
    std::uint32_t escapeBranch=0x00419290u;
    std::uint32_t pauseBranch=0x00419353u;
    std::uint32_t movementPoll=0x00419363u;
    std::uint32_t rightBinding=0x025B7888u;
    std::uint32_t leftBinding=0x025B788Cu;
    std::uint32_t backBinding=0x025B7890u;
    std::uint32_t forwardBinding=0x025B7894u;
    std::array<std::uint32_t,4> movementFlags{{0x0257DAA0u,0x0257DAA4u,0x0257DAA8u,0x0257DAACu}};
    int escapeCode=0x1B;
    int pauseKeyCode=0x50;
    int pauseEventCode=0x13;
    std::array<int,4> arrowCodes{{0x26,0x28,0x25,0x27}}; // up,down,left,right in Win32 VK namespace
};
inline constexpr LegacyGameplayInputTrace kLegacyGameplayInputTrace{};

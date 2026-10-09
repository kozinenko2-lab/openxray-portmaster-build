#pragma once
#include <cstddef>
#include <cstdint>

namespace airxonix {

// DIRECT EXE r149: DirectSound transport around the software mixer.
// The accumulator is 0xAC440 bytes = 0x2B110 DWORD lanes. Each lane represents
// one unsigned-8 output byte and is reset to 128 after it has been mixed.
// 0x40AA10 advances/reset the producer cursor in accumulator BYTES;
// 0x40AA80 consumes one DWORD per output byte and advances the consumer cursor.
// These cursors solve DirectSound lock/play wraparound only; SDL's callback
// already provides a contiguous requested span, so emulating the old ring is
// unnecessary once sample accumulation/quantization is faithful.
struct LegacyAccumulatorRingTrace {
    static constexpr std::uint32_t resetRoutine=0x0040AA10u;
    static constexpr std::uint32_t outputRoutine=0x0040AA80u;
    static constexpr std::uint32_t serviceRoutine=0x0040AAF0u;
    static constexpr std::uint32_t accumulatorBase=0x0046FA6Cu;
    static constexpr std::size_t accumulatorBytes=0x000AC440u;
    static constexpr std::size_t laneBytes=4u;
    static constexpr std::size_t outputBytes=accumulatorBytes/laneBytes;
    static constexpr int neutralLaneValue=128;
    static constexpr std::size_t fallbackLeadBytes=0x1800u;

    static constexpr std::size_t advanceAccumulatorBytes(std::size_t cursor,std::size_t bytes){
        return (cursor + bytes) % accumulatorBytes;
    }
    static constexpr std::size_t advanceOutput(std::size_t accumulatorCursor,std::size_t outputByteCount){
        return advanceAccumulatorBytes(accumulatorCursor,outputByteCount*laneBytes);
    }
};

} // namespace airxonix

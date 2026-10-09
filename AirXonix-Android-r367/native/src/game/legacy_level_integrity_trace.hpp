#pragma once
#include <cstddef>
#include <cstdint>

// DIRECT EXE r111: 0x419139..0x419244 is an integrity/anti-tamper branch,
// not normal gameplay. The checked runtime block is installed through 0x41EC50.
struct LegacyLevelIntegrityTrace {
    static constexpr std::uintptr_t begin=0x00419139u;
    static constexpr std::uintptr_t end=0x00419244u;
    static constexpr std::uintptr_t runtimeBlockSetter=0x0041EC50u;
    static constexpr std::size_t payloadOffset=0x20u;
    static constexpr std::size_t countOffset=0x78u;
    static constexpr std::size_t checksumAOffset=0x5Bu;
    static constexpr std::size_t checksumBOffset=0x5Fu;
    static constexpr std::uint8_t seedA=0x6Bu;
    static constexpr std::uint8_t seedB=0x45u;
    static constexpr std::uint8_t mask=0x3Fu;

    // r263 DIRECT EXE: 0x41EC50 fans out masked runtime-block pointers into
    // globals that sit beside gameplay state but are anti-tamper plumbing.
    // They must not be modeled as score/timer/level variables.
    static constexpr std::uintptr_t maskedRuntimeBlockA=0x0257DA0Cu;
    static constexpr std::uintptr_t maskedRuntimeBlockB=0x0257DA28u;

    static std::uint8_t checksumA(const std::uint8_t* bytes,std::size_t count){
        std::uint8_t v=seedA;
        for(std::size_t i=0;i<count;++i){
            if(i&1u) v=std::uint8_t(v+bytes[i]);
            else v=std::uint8_t(v-bytes[i]);
        }
        return std::uint8_t(v&mask);
    }
    static std::uint8_t checksumB(const std::uint8_t* bytes,std::size_t count){
        std::uint8_t v=seedB;
        for(std::size_t i=0;i<count;++i){
            const std::uint8_t term=std::uint8_t(std::uint8_t(i<<1u)-bytes[i]);
            v=std::uint8_t(v+term);
        }
        return std::uint8_t(v&mask);
    }
};

struct LegacyPlayerGridTrace {
    static constexpr std::uintptr_t prevX=0x0257DA2Cu;
    static constexpr std::uintptr_t prevY=0x0257DA30u;
    static constexpr std::uintptr_t currentX=0x0257DA34u;
    static constexpr std::uintptr_t currentY=0x0257DA38u;
    static constexpr std::uintptr_t interpolationPhase=0x0257DA3Cu;
    static constexpr int initialX=32;
    static constexpr int initialY=-2;
    static constexpr float initialPhase=0.0f;
};

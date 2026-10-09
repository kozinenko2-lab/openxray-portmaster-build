#pragma once
#include <cstdint>

// r279 DIRECT EXE closure. CNT3 is uploaded at atlas #4 (0,220), 256x36 and
// IS consumed at runtime by the capable startup-presentation path 0x41D4D0.
// 0x41DAF8 submits prepared slot1 / LEV2 under texture 7. 0x40EE40 builds a
// decimal mesh from currentLevel using 1 digit below 10 and 2 digits otherwise;
// 0x41DB0F selects texture 4 and 0x41DB58 submits that dynamic mesh. Thus CNT3
// is the decimal strip paired with the LEV2 "Stage:" plaque, not gameplay HUD.
struct LegacyCnt3Trace {
    std::uint32_t fourCC=0x33746E63u; // memory bytes "cnt3"
    int atlas=4;
    int atlasX=0;
    int atlasY=220;
    int width=256;
    int height=36;
    bool hasRuntimeConsumer=true;
    std::uint32_t startupRoutine=0x0041D4D0u;
    std::uint32_t digitBuilder=0x0040EE40u;
    std::uint32_t textureSelect=0x0041DB0Fu;
    std::uint32_t submit=0x0041DB58u;
    std::uint32_t levelGlobal=0x0257DA18u;
    int oneDigitThreshold=10;
};
inline constexpr LegacyCnt3Trace kLegacyCnt3Trace{};

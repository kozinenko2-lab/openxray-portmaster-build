#pragma once
#include <array>
#include <cstdint>

struct LegacyHudNumberCallTrace {
    std::uint32_t valueGlobal;
    std::uint32_t callsite;
    int minDigits;
    std::uint32_t xGlobal;
    std::uint32_t yGlobal;
    int x640;
    int y480;
};

struct LegacyHudStaticQuadTrace {
    const char* resource;
    std::uint32_t buildCallsite;
    int x640;
    int y480;
    int w640;
    int h480;
    float u0;
    float v0;
    float du;
    float dv;
};

// r48 DIRECT EXE gameplay HUD call chain.
// 0x420641 selects texture 3 and 0x42064C calls 0x4247E0(dt).
// 0x4247E0 resets the transient glyph batch through 0x40EB30, emits numeric
// quads through 0x40EC90(value,digits,x,y,colour), then submits via 0x40ED60.
struct LegacyGameplayHudTrace {
    std::uint32_t rendererTexture3Call=0x00420641u;
    std::uint32_t rendererHudCall=0x0042064Cu;
    std::uint32_t routine=0x004247E0u;
    std::uint32_t resetBatch=0x0040EB30u;
    std::uint32_t emitNumber=0x0040EC90u;
    std::uint32_t submitBatch=0x0040ED60u;
    std::uint32_t digitDivisorTable=0x02541934u;
    std::uint32_t digitScratch=0x02541958u;
    std::uint32_t vertexScratch=0x02541978u;
    std::uint32_t glyphU0Table=0x0254190Cu;
    std::uint32_t glyphU1Table=0x02541910u;
    std::uint32_t glyphCount=10;
    // 640x480 values produced by the high-resolution layout initializer
    // at 0x4241D0..0x4242ED. These are raw legacy destination coordinates.
    std::array<int,10> layout640x480{{32,4,576,4,64,460,121,460,544,460}};
    std::uint32_t layoutInitHighRes=0x004241D0u;
    std::uint32_t staticQuadBuilder=0x0040EB40u;
    std::uint32_t numericInitRoutine=0x0040E9C0u;
    int glyphAdvance640=16;
    int glyphHeight640=16;
};
inline constexpr LegacyGameplayHudTrace kLegacyGameplayHudTrace{};

// Raw numeric callsites. Semantic names are intentionally limited to globals
// already proven elsewhere; exact icon/label pairing is kept separate.
inline constexpr std::array<LegacyHudNumberCallTrace,5> kLegacyGameplayHudNumberCalls{{
    {0x0257DA10u,0x00424905u,1,0x025B5B68u,0x025B5B6Cu,32,4},
    {0x0257DA18u,0x00424941u,2,0x025B5B84u,0x025B5B88u,64,460},
    {0x025B5B98u,0x0042497Bu,2,0x025B5B74u,0x025B5B78u,576,4},
    {0x0257DA1Cu,0x004249DFu,2,0x025B5B8Cu,0x025B5B90u,121,460},
    {0x025B5B94u,0x00424A1Au,6,0x025B5B7Cu,0x025B5B80u,544,460},
}};


// r49 DIRECT EXE: 0x424331..0x4244D2 builds five static texture-3 HUD quads
// through 0x40EB40. Their UV signatures identify the resources unambiguously.
// Coordinates below are the exact high-resolution 640x480 branch values.
inline constexpr std::array<LegacyHudStaticQuadTrace,5> kLegacyGameplayHudStaticQuads{{
    {"HEAR",0x0042435Du,0,0,32,32,64.5f/256.f,32.5f/256.f,31.f/256.f,31.f/256.f},
    {"CLCK",0x00424397u,608,0,32,32,96.5f/256.f,32.5f/256.f,31.f/256.f,31.f/256.f},
    {"LEVL",0x00424401u,0,460,64,20,129.f/256.f,34.f/256.f,62.f/256.f,20.f/256.f},
    {"SCOR",0x0042446Cu,524,460,20,20,195.f/256.f,34.f/256.f,20.f/256.f,20.f/256.f},
    {"PERC",0x004244D2u,96,460,26,20,216.5f/256.f,33.f/256.f,23.f/256.f,22.f/256.f},
}};

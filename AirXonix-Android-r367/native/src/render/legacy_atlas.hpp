#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

// Reconstructed Direct3D RGB565 texture-surface layout. 0x4058F0 proves that
// each entry is exactly {FourCC, atlasIndex, x, y}; the source RGB565 rows are
// copied directly into the locked D3D surface without colour conversion.
// The GLES2 port can therefore rebuild the same atlases from unpacked PNGs and
// retain legacy UV coordinates while leaving source files editable.
struct LegacyAtlasSize { int width,height; };
struct LegacyAtlasEntry { const char* fourcc; int atlas,x,y; };


// 0x4058F0 is the legacy atlas copier. It consumes a sentinel-terminated
// array of 16-byte records {FourCC, atlasIndex, dstX, dstY}, resolves each
// source bitmap with 0x409D80, locks the destination D3D surface and copies
// the source rows at their native width/height. Therefore resources such as
// SHAD have no post-construction FourCC "consumer"; later code addresses the
// atlas by texture index and UVs.
struct LegacyAtlasCopierContract {
    std::uint32_t copierAddress;
    std::uint32_t resolverAddress;
    std::size_t recordSize;
    bool sentinelFourccZero;
};
inline constexpr LegacyAtlasCopierContract kLegacyAtlasCopier{
    0x004058F0u, 0x00409D80u, 16u, true
};

// Source extents verified from the original unpacked BMPPACK payload. These
// are source-image sizes, not inferred free space in the destination atlas.
struct LegacyAtlasSourceExtent { const char* fourcc; int width,height; };
inline constexpr std::array<LegacyAtlasSourceExtent,8> kLegacyHudSourceExtents{{
    {"SHAD",16,16}, {"CNT3",256,36}, {"LEV2",256,64}, {"HEAR",32,32},
    {"CLCK",32,32}, {"SCOR",24,24}, {"PERC",24,24}, {"PAUS",64,24}
}};

// r156 direct-EXE audit: SHAD is copied into atlas #3 at (0,64), but the
// final executable has no draw consumer for that 16x16 rectangle. All five
// occurrences of the distinctive 0.251953125 half-texel coordinate belong
// to PAUS/M2/HUD builders using other atlas regions. Keep SHAD only because
// the original atlas constructor uploads it and its footprint affects layout.
struct LegacyAtlasRect { int atlas,x,y,w,h; };
inline constexpr LegacyAtlasRect kLegacyShadRect{3,0,64,16,16};
inline constexpr bool kLegacyShadHasDrawConsumer=false;

inline constexpr std::array<LegacyAtlasSize,9> kLegacyAtlasSizes{{
    {64,64},{64,64},{64,64},
    {256,256},{256,256},{256,256},
    {128,128},{256,256},{256,256}
}};

// Static portion of the gameplay/HUD table built by 0x423350/0x4237C0.
// The two functions are identical except COMP/GAME vs cmp2/gam2. Entries
// before BALL contain runtime-selected environment resources and are kept out
// of this static table until those globals are mapped semantically.
inline constexpr std::array<LegacyAtlasEntry,29> kLegacyGameplayAtlasBase{{
    {"BALL",3,0,0}, {"XONI",3,32,0}, {"SPEE",3,0,32}, {"MONY",3,32,32},
    {"XON1",3,0,224}, {"VZRV",3,240,32}, {"CNT2",3,64,0}, {"HEAR",3,64,32},
    {"CLCK",3,96,32}, {"LEVL",3,128,32}, {"SCOR",3,192,32}, {"PERC",3,216,32},
    {"IN2$",7,0,0}, {"IN2T",7,128,0}, {"IN2L",7,0,48}, {"IN2S",7,128,48},
    {"TOU2",7,0,144},
    {"PAUS",3,64,112}, {"GOVE",4,0,0}, {"ABOR",4,0,96}, {"IN2A",7,0,96},
    {"VZR1",3,192,160}, {"RAM3",4,128,128}, {"1111",6,0,0}, {"SHAD",3,0,64},
    {"LEV2",7,0,192}, {"CNT3",4,0,220},
    // The following optional resource is referenced by the executable but is
    // absent from the analysed BMPPACK; 0x4058F0 simply skips missing FourCCs.
    {"TXR8",2,0,0},
    // Sentinel-like generated/reference texture seen in the same table.
    {"NREG",7,0,0}
}};

inline constexpr std::array<LegacyAtlasEntry,2> kLegacyGameplayUiVariantA{{
    {"COMP",4,0,48}, {"GAME",4,0,128}
}};
inline constexpr std::array<LegacyAtlasEntry,2> kLegacyGameplayUiVariantB{{
    {"cmp2",4,0,48}, {"gam2",4,0,128}
}};

// Static table built by 0x423C30. This is a separate screen/resource set and
// contains the first M1xx strip plus logo/background assets.
inline constexpr std::array<LegacyAtlasEntry,17> kLegacyAtlasSetM1{{
    {"TEMP",3,0,156}, {"fnt4",5,0,0},
    {"BALL",3,0,0}, {"XONI",3,32,0}, {"SPEE",3,0,32}, {"MONY",3,32,32},
    {"AAAA",3,0,192}, {"RRRR",3,64,192},
    {"M101",4,0,0}, {"M102",4,0,48}, {"M103",4,0,96}, {"M104",4,0,144}, {"M106",4,0,192},
    {"VZRV",3,240,32}, {"1111",6,0,0}, {"LOGO",8,0,0}, {"LAXY",2,0,0}
}};

// Static table built by 0x423EE0. The executable references FONT although the
// analysed pack exposes fnt4; missing-resource behaviour is intentionally
// preserved. M2xx are packed vertically in atlas 4 in 42-pixel steps.
inline constexpr std::array<LegacyAtlasEntry,20> kLegacyAtlasSetM2{{
    {"TEMP",3,0,156}, {"FONT",3,0,160},
    {"BALL",3,0,0}, {"XONI",3,32,0}, {"SPEE",3,0,32}, {"MONY",3,32,32},
    {"AAAA",3,0,192}, {"RRRR",3,64,192},
    {"M250",4,0,0}, {"M260",4,0,42}, {"M240",4,0,84}, {"M210",4,0,126},
    {"M220",4,0,168}, {"M230",4,0,210},
    {"M102",3,0,72}, {"on++",3,128,0}, {"off+",3,192,0},
    {"NREG",7,0,0}, {"1111",6,0,0}, {"fnt4",5,0,0}
}};


// Constructor-family metadata. These records deliberately describe only facts
// proven by table construction, not menu semantics. They let the runtime and
// tests preserve the two atlas-#4 families until xrefs from 0x412F90 identify
// which family belongs to which screen/state.
enum class LegacyAtlasConstructorFamily { GameplayHud, MenuM1, MenuM2 };
struct LegacyAtlasConstructorInfo {
    std::uint32_t address;
    LegacyAtlasConstructorFamily family;
    int rebuiltAtlas;
    std::size_t entryCount;
};
inline constexpr std::array<LegacyAtlasConstructorInfo,4> kLegacyAtlasConstructors{{
    {0x00423350u,LegacyAtlasConstructorFamily::GameplayHud,4,kLegacyGameplayAtlasBase.size()+kLegacyGameplayUiVariantA.size()},
    {0x004237C0u,LegacyAtlasConstructorFamily::GameplayHud,4,kLegacyGameplayAtlasBase.size()+kLegacyGameplayUiVariantB.size()},
    {0x00423C30u,LegacyAtlasConstructorFamily::MenuM1,4,kLegacyAtlasSetM1.size()},
    {0x00423EE0u,LegacyAtlasConstructorFamily::MenuM2,4,kLegacyAtlasSetM2.size()}
}};

#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace airxonix { class LegacyOriginalResources; }

struct LegacyAtlasImage {
    int width=0;
    int height=0;
    std::vector<std::uint8_t> rgba;
};

namespace LegacyAtlasRuntime {
// Rebuilds the recovered 256x256 gameplay atlas (legacy atlas #3) from the
// ordinary editable PNG files in assets/textures. Missing tiles are left black.
bool loadPngImage(const std::string& path,LegacyAtlasImage& out,std::string* error=nullptr);
// Loose-first texture loader. If assets/textures/<FourCC>.png is absent,
// falls back to the original BMPPACK embedded in AirXonix.wrp.exe.
bool loadTexture(const std::string& texturesPath,const std::string& fourcc,const airxonix::LegacyOriginalResources* packed,LegacyAtlasImage& out,std::string* error=nullptr);
bool buildGameplayAtlas3(const std::string& texturesPath,LegacyAtlasImage& out,std::string* error=nullptr,const airxonix::LegacyOriginalResources* packed=nullptr);
// Rebuilds legacy UI atlas #4 (game-over/completion/abort presentation).
bool buildUiAtlas4(const std::string& texturesPath,LegacyAtlasImage& out,std::string* error=nullptr,const airxonix::LegacyOriginalResources* packed=nullptr);
// Original EXE has two mutually-exclusive constructors for atlas #4:
// COMP+GAME = "ПРОЙДЕН"+"ЭТАП" and cmp2+gam2 = "ПРОЙДЕНА"+"ИГРА".
bool buildUiAtlas4GameComplete(const std::string& texturesPath,LegacyAtlasImage& out,std::string* error=nullptr,const airxonix::LegacyOriginalResources* packed=nullptr);
// Rebuilds the recovered 256x256 auxiliary gameplay/screen atlas #7.
// This contains LEV2 and the IN2*/TOU2 instruction resources referenced by
// the same 0x423350/0x4237C0 constructor family as CNT3.
bool buildAuxAtlas7(const std::string& texturesPath,LegacyAtlasImage& out,std::string* error=nullptr,const airxonix::LegacyOriginalResources* packed=nullptr);
// Phase-specific atlas #4 layouts from the two dedicated screen constructors.
// These are intentionally not auto-bound to MainMenu until the 0x412F90
// constructor callsites are re-traced.
bool buildMenuAtlas4M1(const std::string& texturesPath,LegacyAtlasImage& out,std::string* error=nullptr,const airxonix::LegacyOriginalResources* packed=nullptr);
bool buildMenuAtlas3M1(const std::string& texturesPath,LegacyAtlasImage& out,std::string* error=nullptr,const airxonix::LegacyOriginalResources* packed=nullptr);
bool buildMenuAtlas3M2(const std::string& texturesPath,LegacyAtlasImage& out,std::string* error=nullptr,const airxonix::LegacyOriginalResources* packed=nullptr);
bool buildMenuAtlas4M2(const std::string& texturesPath,LegacyAtlasImage& out,std::string* error=nullptr,const airxonix::LegacyOriginalResources* packed=nullptr);
}

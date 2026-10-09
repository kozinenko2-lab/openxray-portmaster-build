#include "legacy_atlas_runtime.hpp"
#include "core/legacy_original_resources.hpp"
#include "core/cleanroom_archive.hpp"
#include "builtin_resources.hpp"
#ifndef AIRXONIX_NO_PNG
#include <png.h>
#endif
#include <array>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace LegacyAtlasRuntime {
namespace {
struct Tile { const char* name; int x; int y; };
constexpr std::array<Tile,15> kTiles={{
    {"BALL",0,0},{"XONI",32,0},{"SPEE",0,32},{"MONY",32,32},
    {"XON1",0,224},{"VZRV",240,32},{"CNT2",64,0},{"HEAR",64,32},
    {"CLCK",96,32},{"LEVL",128,32},{"SCOR",192,32},{"PERC",216,32},
    {"PAUS",64,112},{"VZR1",192,160},{"SHAD",0,64}
}};
constexpr std::array<Tile,6> kUiTilesStage={{
    {"GOVE",0,0},{"COMP",0,48},{"ABOR",0,96},
    {"GAME",0,128},{"RAM3",128,128},{"CNT3",0,220}
}};
constexpr std::array<Tile,6> kUiTilesGameComplete={{
    {"GOVE",0,0},{"cmp2",0,48},{"ABOR",0,96},
    {"gam2",0,128},{"RAM3",128,128},{"CNT3",0,220}
}};
constexpr std::array<Tile,7> kAuxTiles={{
    {"IN2$",0,0},{"IN2T",128,0},{"IN2L",0,48},{"IN2S",128,48},
    {"IN2A",0,96},{"TOU2",0,144},{"LEV2",0,192}
}};
constexpr std::array<Tile,5> kMenuM1Tiles={{
    {"M101",0,0},{"M102",0,48},{"M103",0,96},{"M104",0,144},{"M106",0,192}
}};
// r194 DIRECT EXE 0x423C30..0x423E81: logical texture 3 entries of the M1
// constructor. It does not contain XON1/HUD tiles; AAAA/RRRR at y=192 are the
// faces of menu slot 4 (0x42288D/0x4228BC cap UV centres (32,226),(96,224)).
constexpr std::array<Tile,8> kMenuM1Atlas3Tiles={{
    {"TEMP",0,156},{"BALL",0,0},{"XONI",32,0},{"SPEE",0,32},{"MONY",32,32},
    {"AAAA",0,192},{"RRRR",64,192},{"VZRV",240,32}
}};
// r194: logical texture 3 entries of the M2 constructor 0x423EE0 (settings /
// controls). on++/off+ (speech toggle) and M102 live here, not in the gameplay
// atlas; FONT is referenced but absent from BMPPACK.
constexpr std::array<Tile,10> kMenuM2Atlas3Tiles={{
    {"TEMP",0,156},{"BALL",0,0},{"XONI",32,0},{"SPEE",0,32},{"MONY",32,32},
    {"AAAA",0,192},{"RRRR",64,192},{"M102",0,72},{"on++",128,0},{"off+",192,0}
}};
constexpr std::array<Tile,6> kMenuM2Tiles={{
    {"M250",0,0},{"M260",0,42},{"M240",0,84},{"M210",0,126},{"M220",0,168},{"M230",0,210}
}};


bool decodeTgaBytes(const std::vector<std::uint8_t>& bytes,const std::string& label,int& width,int& height,std::vector<std::uint8_t>& rgba,std::string* error){
    if(bytes.size()<18u){if(error)*error="truncated TGA header: "+label;return false;}
    const auto* h=bytes.data();
    const int idBytes=h[0];
    const int colorMapType=h[1];
    const int imageType=h[2];
    const int w=int(h[12])|(int(h[13])<<8);
    const int hh=int(h[14])|(int(h[15])<<8);
    const int bpp=h[16];
    if(colorMapType!=0 || imageType!=2 || w<=0 || hh<=0 || (bpp!=24 && bpp!=32)){
        if(error)*error="unsupported clean-room TGA (need uncompressed 24/32-bit truecolour): "+label;
        return false;
    }
    const int srcBytes=bpp/8;
    const std::size_t pixelOffset=18u+std::size_t(idBytes);
    const std::size_t need=std::size_t(w)*std::size_t(hh)*std::size_t(srcBytes);
    if(pixelOffset>bytes.size() || need>bytes.size()-pixelOffset){if(error)*error="truncated TGA pixels: "+label;return false;}
    const auto* src=bytes.data()+pixelOffset;
    width=w;height=hh;rgba.assign(std::size_t(w)*std::size_t(hh)*4u,0u);
    const bool topOrigin=(h[17]&0x20u)!=0u;
    const bool rightOrigin=(h[17]&0x10u)!=0u;
    for(int sy=0;sy<hh;++sy){
        const int dy=topOrigin?sy:(hh-1-sy);
        for(int sx=0;sx<w;++sx){
            const int dx=rightOrigin?(w-1-sx):sx;
            const auto* sp=src+(std::size_t(sy)*std::size_t(w)+std::size_t(sx))*std::size_t(srcBytes);
            auto* dp=rgba.data()+(std::size_t(dy)*std::size_t(w)+std::size_t(dx))*4u;
            dp[0]=sp[2];dp[1]=sp[1];dp[2]=sp[0];dp[3]=srcBytes==4?sp[3]:255u;
        }
    }
    return true;
}

bool readTgaInternal(const std::string& path,int& width,int& height,std::vector<std::uint8_t>& rgba,std::string* error){
    std::ifstream f(path,std::ios::binary);
    if(!f){if(error)*error="cannot open "+path;return false;}
    std::vector<std::uint8_t> bytes(std::istreambuf_iterator<char>(f),{});
    return decodeTgaBytes(bytes,path,width,height,rgba,error);
}

bool readPngInternal(const std::string& path,int& width,int& height,std::vector<std::uint8_t>& rgba,std::string* error){
#ifdef AIRXONIX_NO_PNG
    (void)width; (void)height; (void)rgba;
    if(error)*error="PNG override disabled in resource-free console build: "+path;
    return false;
#else
    FILE* fp=std::fopen(path.c_str(),"rb");
    if(!fp){ if(error)*error="cannot open "+path; return false; }
    png_structp png=png_create_read_struct(PNG_LIBPNG_VER_STRING,nullptr,nullptr,nullptr);
    png_infop info=png?png_create_info_struct(png):nullptr;
    if(!png||!info){ if(png)png_destroy_read_struct(&png,nullptr,nullptr); std::fclose(fp); if(error)*error="libpng allocation failed"; return false; }
    if(setjmp(png_jmpbuf(png))){ png_destroy_read_struct(&png,&info,nullptr); std::fclose(fp); if(error)*error="libpng decode failed: "+path; return false; }
    png_init_io(png,fp); png_read_info(png,info);
    width=static_cast<int>(png_get_image_width(png,info));
    height=static_cast<int>(png_get_image_height(png,info));
    const png_byte color=png_get_color_type(png,info); const png_byte depth=png_get_bit_depth(png,info);
    if(depth==16)png_set_strip_16(png);
    if(color==PNG_COLOR_TYPE_PALETTE)png_set_palette_to_rgb(png);
    if(color==PNG_COLOR_TYPE_GRAY && depth<8)png_set_expand_gray_1_2_4_to_8(png);
    if(png_get_valid(png,info,PNG_INFO_tRNS))png_set_tRNS_to_alpha(png);
    if(color==PNG_COLOR_TYPE_GRAY || color==PNG_COLOR_TYPE_GRAY_ALPHA)png_set_gray_to_rgb(png);
    if(!(color & PNG_COLOR_MASK_ALPHA))png_set_add_alpha(png,0xff,PNG_FILLER_AFTER);
    png_read_update_info(png,info);
    rgba.resize(static_cast<std::size_t>(width)*static_cast<std::size_t>(height)*4u);
    std::vector<png_bytep> rows(static_cast<std::size_t>(height));
    for(int y=0;y<height;++y) rows[static_cast<std::size_t>(y)]=rgba.data()+static_cast<std::size_t>(y)*static_cast<std::size_t>(width)*4u;
    png_read_image(png,rows.data());
    png_destroy_read_struct(&png,&info,nullptr); std::fclose(fp); return true;
#endif
}
}


bool loadPngImage(const std::string& path,LegacyAtlasImage& out,std::string* error){
    out={};
    return readPngInternal(path,out.width,out.height,out.rgba,error);
}

bool loadTexture(const std::string& texturesPath,const std::string& fourcc,const airxonix::LegacyOriginalResources* packed,LegacyAtlasImage& out,std::string* error){
    // Explicit PNGs remain developer overrides in non-resource-free builds.
    std::string pngError;
    if(readPngInternal(texturesPath+"/"+fourcc+".png",out.width,out.height,out.rgba,&pngError)) return true;
    // When the commercial executable is present it remains the highest-fidelity
    // normal source. The clean-room package is used automatically only when the
    // original texture is unavailable or the EXE is absent.
    std::string packedError;
    if(packed && packed->isOpen()){
        airxonix::LegacyPackedImage image;
        if(packed->loadTexture(fourcc,image,&packedError)){out.width=image.width;out.height=image.height;out.rgba=std::move(image.rgba);return true;}
    }
    // r205 standalone pack: uncompressed TGA is intentionally supported in the
    // resource-free console build so no libpng/shared-library dependency is
    // required on H700/RK3326 firmware.
    std::string tgaError;
    std::vector<std::uint8_t> zippedTga;
    if(airxonix::CleanroomArchive::instance().read("textures/"+fourcc+".tga",zippedTga) &&
       decodeTgaBytes(zippedTga,"cleanroom.zip:textures/"+fourcc+".tga",out.width,out.height,out.rgba,&tgaError)) return true;
    // Keep loose TGA only as a developer/source-tree fallback. Runtime packages
    // carry the clean-room namespace exclusively inside AirXonix-cleanroom.zip.
    if(readTgaInternal(texturesPath+"/"+fourcc+".tga",out.width,out.height,out.rgba,&tgaError)) return true;
    // Last-resort emergency resources are compiled into the executable. This
    // means a damaged/missing clean-room texture pack still cannot block boot.
    BuiltinResources::buildTexture(fourcc,out);
    if(error){
        *error="using built-in emergency replacement for "+fourcc;
        if(!tgaError.empty())*error+="; "+tgaError;
        if(!packedError.empty())*error+="; "+packedError;
        if(!pngError.empty())*error+="; "+pngError;
    }
    return true;
}

template <std::size_t N>
bool buildTileAtlas(const std::string& texturesPath,const std::array<Tile,N>& tiles,
                    const char* label,LegacyAtlasImage& out,std::string* error,const airxonix::LegacyOriginalResources* packed){
    out.width=256; out.height=256; out.rgba.assign(256u*256u*4u,0u);
    bool any=false;
    for(const Tile& t:tiles){
        int w=0,h=0; std::vector<std::uint8_t> src; std::string local;
        LegacyAtlasImage tile; if(!loadTexture(texturesPath,t.name,packed,tile,&local))continue; w=tile.width;h=tile.height;src=std::move(tile.rgba);
        any=true;
        const int copyW=(t.x+w>256)?(256-t.x):w;
        const int copyH=(t.y+h>256)?(256-t.y):h;
        for(int y=0;y<copyH;++y){
            const auto* srcRow=src.data()+static_cast<std::size_t>(y)*static_cast<std::size_t>(w)*4u;
            auto* dst=out.rgba.data()+(static_cast<std::size_t>(t.y+y)*256u+static_cast<std::size_t>(t.x))*4u;
            std::memcpy(dst,srcRow,static_cast<std::size_t>(copyW)*4u);
            for(int x=0;x<copyW;++x){auto* px=dst+static_cast<std::size_t>(x)*4u;if(px[0]==0&&px[1]==0&&px[2]==0)px[3]=0;}
        }
    }
    if(!any && error)*error=std::string("no ")+label+" PNG tiles found in "+texturesPath;
    return any;
}

bool buildGameplayAtlas3(const std::string& texturesPath,LegacyAtlasImage& out,std::string* error,const airxonix::LegacyOriginalResources* packed){
    out.width=256; out.height=256; out.rgba.assign(256u*256u*4u,0u);
    bool any=false;
    for(const Tile& t:kTiles){
        int w=0,h=0; std::vector<std::uint8_t> src; std::string local;
        LegacyAtlasImage tile; if(!loadTexture(texturesPath,t.name,packed,tile,&local)) continue; w=tile.width;h=tile.height;src=std::move(tile.rgba); // loose PNG or original BMPPACK
        any=true;
        const int copyW=(t.x+w>256)?(256-t.x):w; const int copyH=(t.y+h>256)?(256-t.y):h;
        for(int y=0;y<copyH;++y){
            const auto* s=src.data()+static_cast<std::size_t>(y)*static_cast<std::size_t>(w)*4u;
            auto* d=out.rgba.data()+(static_cast<std::size_t>(t.y+y)*256u+static_cast<std::size_t>(t.x))*4u;
            std::memcpy(d,s,static_cast<std::size_t>(copyW)*4u);
            // BMPPACK is RGB565 and therefore has no alpha channel.  The
            // original D3D7 sprite/model path uses black as transparent color
            // key; preserve that behaviour in RGBA atlases for GLES2.
            for(int x=0;x<copyW;++x){auto* px=d+static_cast<std::size_t>(x)*4u;if(px[0]==0&&px[1]==0&&px[2]==0)px[3]=0;}
        }
    }
    if(!any && error)*error="no gameplay PNG tiles found in "+texturesPath;
    return any;
}

template<std::size_t N>
static bool buildUiAtlas4From(const std::string& texturesPath,const std::array<Tile,N>& tiles,
                              LegacyAtlasImage& out,std::string* error,
                              const airxonix::LegacyOriginalResources* packed){
    out.width=256; out.height=256; out.rgba.assign(256u*256u*4u,0u);
    bool any=false;
    for(const Tile& t:tiles){
        int w=0,h=0; std::vector<std::uint8_t> src; std::string local;
        LegacyAtlasImage tile; if(!loadTexture(texturesPath,t.name,packed,tile,&local))continue;
        w=tile.width;h=tile.height;src=std::move(tile.rgba); any=true;
        const int copyW=(t.x+w>256)?(256-t.x):w; const int copyH=(t.y+h>256)?(256-t.y):h;
        for(int y=0;y<copyH;++y){
            const auto* srcRow=src.data()+static_cast<std::size_t>(y)*static_cast<std::size_t>(w)*4u;
            auto* dst=out.rgba.data()+(static_cast<std::size_t>(t.y+y)*256u+static_cast<std::size_t>(t.x))*4u;
            std::memcpy(dst,srcRow,static_cast<std::size_t>(copyW)*4u);
            for(int x=0;x<copyW;++x){auto* px=dst+static_cast<std::size_t>(x)*4u;if(px[0]==0&&px[1]==0&&px[2]==0)px[3]=0;}
        }
    }
    if(!any && error)*error="no UI PNG tiles found in "+texturesPath;
    return any;
}

bool buildUiAtlas4(const std::string& texturesPath,LegacyAtlasImage& out,std::string* error,const airxonix::LegacyOriginalResources* packed){
    return buildUiAtlas4From(texturesPath,kUiTilesStage,out,error,packed);
}

bool buildUiAtlas4GameComplete(const std::string& texturesPath,LegacyAtlasImage& out,std::string* error,const airxonix::LegacyOriginalResources* packed){
    return buildUiAtlas4From(texturesPath,kUiTilesGameComplete,out,error,packed);
}

bool buildAuxAtlas7(const std::string& texturesPath,LegacyAtlasImage& out,std::string* error,const airxonix::LegacyOriginalResources* packed){
    return buildTileAtlas(texturesPath,kAuxTiles,"auxiliary atlas #7",out,error,packed);
}

bool buildMenuAtlas4M1(const std::string& texturesPath,LegacyAtlasImage& out,std::string* error,const airxonix::LegacyOriginalResources* packed){
    return buildTileAtlas(texturesPath,kMenuM1Tiles,"M1xx menu atlas #4",out,error,packed);
}

bool buildMenuAtlas3M1(const std::string& texturesPath,LegacyAtlasImage& out,std::string* error,const airxonix::LegacyOriginalResources* packed){
    return buildTileAtlas(texturesPath,kMenuM1Atlas3Tiles,"M1 menu atlas #3",out,error,packed);
}

bool buildMenuAtlas3M2(const std::string& texturesPath,LegacyAtlasImage& out,std::string* error,const airxonix::LegacyOriginalResources* packed){
    return buildTileAtlas(texturesPath,kMenuM2Atlas3Tiles,"M2 menu atlas #3",out,error,packed);
}

bool buildMenuAtlas4M2(const std::string& texturesPath,LegacyAtlasImage& out,std::string* error,const airxonix::LegacyOriginalResources* packed){
    return buildTileAtlas(texturesPath,kMenuM2Tiles,"M2xx menu atlas #4",out,error,packed);
}

}

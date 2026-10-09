#include "render/builtin_resources.hpp"
#include "render/legacy_atlas_runtime.hpp"
#include <array>
#include <cassert>
#include <iostream>

struct Expected { const char* name; int w,h; };
int main(){
    constexpr std::array<Expected,31> k{{
        {"BALL",32,32},{"CLCK",32,32},{"HEAR",32,32},{"MONY",32,32},{"SPEE",32,32},{"XONI",32,32},
        {"CNT2",192,24},{"CNT3",256,36},{"LEVL",64,24},{"SCOR",24,24},{"PERC",24,24},{"PAUS",64,24},
        {"XON1",256,32},{"VZRV",16,16},{"VZR1",64,64},{"SHAD",16,16},
        {"GOVE",256,48},{"COMP",256,48},{"cmp2",256,48},{"ABOR",256,32},{"GAME",128,48},{"gam2",128,48},
        {"RAM3",128,92},{"IN2A",128,48},{"TOU2",256,48},{"LEV2",256,64},{"1111",128,128},
        {"LOGO",256,256},{"fnt4",256,256},{"on++",64,38},{"off+",64,38}
    }};
    for(const auto& e:k){ LegacyAtlasImage img; assert(BuiltinResources::buildTexture(e.name,img)); assert(img.width==e.w); assert(img.height==e.h); }
    LegacyAtlasImage temp; assert(BuiltinResources::buildTexture("TEMP",temp)); assert(temp.width==256 && temp.height==4);
    LegacyAtlasImage m1,m2; assert(BuiltinResources::buildTexture("M101",m1)); assert(m1.width==256&&m1.height==48);
    assert(BuiltinResources::buildTexture("M250",m2)); assert(m2.width==256&&m2.height==42);
    std::cout << "all known non-64 BMPPACK fallback extents match original\n";
}

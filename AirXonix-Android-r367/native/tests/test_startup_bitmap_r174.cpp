#include "core/legacy_original_resources.hpp"
#include <cassert>
#include <cstdio>
int main(int argc,char** argv){
    if(argc<2)return 77;
    airxonix::LegacyOriginalResources r;std::string e;assert(r.open(argv[1],&e));
    airxonix::LegacyPackedImage img;assert(r.loadStartupBitmap143(img,&e));
    assert(img.width==67&&img.height==134);assert(img.rgba.size()==67u*134u*4u);
    std::size_t nonBlack=0;for(std::size_t i=0;i<img.rgba.size();i+=4)if(img.rgba[i]||img.rgba[i+1]||img.rgba[i+2])++nonBlack;
    assert(nonBlack>1000);std::puts("startup bitmap r174 ok");
}

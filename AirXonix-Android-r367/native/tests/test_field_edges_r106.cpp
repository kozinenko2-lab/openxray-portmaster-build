#include "render/legacy_field_tessellators.hpp"
#include <cassert>
#include <iostream>
using namespace LegacyFieldTessellators;
int main(){
    std::array<std::uint8_t,W*H> f{};
    f.fill(0);
    // SAFE->non-safe is accepted in left 48-column window.
    f[10*W+20]=Safe; f[10*W+21]=0;
    // Same orientation outside x=0..47 must be culled by 0x41F660.
    f[10*W+55]=Safe; f[10*W+56]=0;
    // non-safe->SAFE accepted only when left cell x>=15.
    f[20*W+30]=0; f[20*W+31]=Safe;
    f[20*W+5]=0; f[20*W+6]=Safe;
    auto e=highHorizontalTransitions(f);
    bool a=false,b=false,c=false,d=false;
    for(auto q:e){
        if(q.y==10&&q.x==21)a=true;
        if(q.y==10&&q.x==56)b=true;
        if(q.y==20&&q.x==31)c=true;
        if(q.y==20&&q.x==6)d=true;
    }
    assert(a&&!b&&c&&!d);
    assert(highWallXVisible(47,10,true)); assert(!highWallXVisible(48,10,true));
    assert(highWallXVisible(16,10,false)); assert(!highWallXVisible(15,10,false));
    assert(highWallZVisible(1,1)&&highWallZVisible(62,63));
    assert(!highWallZVisible(0,20)&&!highWallZVisible(20,64));
    std::cout<<"field edges r106 ok\n";
}

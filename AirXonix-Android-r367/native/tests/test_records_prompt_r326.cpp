#include "render/legacy_hud.hpp"
#include "render/legacy_theme.hpp"
#include <cassert>
#include <iostream>
int main(){
    LegacyHudState s{}; s.screen=LegacyHudScreen::Records; s.recordsNameEntry=true;
    s.recordsCandidateRow=0; s.recordsNamePulseByte=191;
    const auto q=LegacyHud::compose(s);
    int count=0;
    const float gray=191.f/255.f;
    for(const auto& g:q){
        if(g.y==14.f*32.f && g.x>=12.f*16.f && g.x<28.f*16.f){
            ++count; assert(g.r==gray && g.g==gray && g.b==gray);
        }
    }
    assert(count==16);
    std::cout<<"records prompt r326 PASS\n";
}

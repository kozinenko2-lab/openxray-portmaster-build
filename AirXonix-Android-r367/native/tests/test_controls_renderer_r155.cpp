#include "render/legacy_hud.hpp"
#include "render/legacy_theme.hpp"
#include <cassert>
#include <iostream>
int main(){
    LegacyHudState s; s.screen=LegacyHudScreen::Controls;
    s.controlsBindings={{0x41,0x5A,0x104,0x118}};
    const auto q=LegacyHud::compose(s);
    assert(!q.empty());
    int font=0; bool sawA=false,sawJ=false;
    for(const auto& v:q){
        assert(v.atlas==LegacyHudAtlas::Font5); ++font;
        assert(v.u0>=0.f && v.v0>=0.f && v.u1>v.u0 && v.v1>v.v0);
        if(v.y==7.f*32.f && v.x>=17.f*16.f)sawA=true;
        if(v.y==9.f*32.f && v.x>=17.f*16.f)sawJ=true;
    }
    assert(font>100 && sawA && sawJ);
    s.controlsAwaitingConfirm=true;
    const auto q2=LegacyHud::compose(s);
    assert(q2.size()>q.size());
    std::cout<<"r155 controls fnt4 renderer commands ok\n";
}

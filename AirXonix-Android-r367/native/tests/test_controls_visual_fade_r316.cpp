#include "render/legacy_hud.hpp"
#include <cassert>
#include <iostream>
int main(){LegacyHudState s{};s.screen=LegacyHudScreen::Controls;s.controlsFadeCounter=0x3e0;auto q=LegacyHud::compose(s);assert(!q.empty());bool any=false;for(const auto& x:q){if(x.r>0.f||x.g>0.f||x.b>0.f){any=true;break;}}assert(any);std::cout<<"controls visual fade r316 PASS\n";}

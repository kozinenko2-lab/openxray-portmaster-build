#include "render/legacy_controls_visual.hpp"
#include <cassert>
#include <string>
#include <iostream>
static std::string s(const std::array<char,8>& a){
    std::string out(a.data(),a.size());
    while(!out.empty() && (out.back()==' ' || out.back()=='\0')) out.pop_back();
    return out;
}
int main(){
    static_assert(LegacyControlsVisual::kTrace.routine==0x00410D60u);
    static_assert(LegacyControlsVisual::kTrace.textureSlot==5);
    assert(s(LegacyControlsVisual::keyName(0x41))=="A");
    assert(s(LegacyControlsVisual::keyName(0x70)).rfind("F1",0)==0);
    assert(s(LegacyControlsVisual::keyName(0x10)).rfind("SHIFT",0)==0);
    assert(s(LegacyControlsVisual::keyName(0x104)).rfind("J1-but1",0)==0);
    assert(s(LegacyControlsVisual::keyName(0x118)).rfind("J2-Right",0)==0);
    std::cout<<"r155 controls grid/key-name contract ok\n";
}

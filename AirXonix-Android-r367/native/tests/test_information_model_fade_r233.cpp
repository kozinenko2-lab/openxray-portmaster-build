#include "game/legacy_information_transition.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

int main(){
    using namespace LegacyInformationTransition;
    assert(modelLightByte(0)==0);
    assert(modelLightByte(7)==0);
    assert(modelLightByte(8)==1);
    assert(modelLightByte(FadeMax)==248);
    assert(std::fabs(modelLightScale(FadeMax)-(248.f/255.f))<1e-7f);
    assert(modelLightByte(FadeMax+1000)==248);
    std::cout << "information model fade r233 PASS\n";
}

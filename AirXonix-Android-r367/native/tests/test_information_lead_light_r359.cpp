#include "game/legacy_information_transition.hpp"
#include <cassert>
int main(){
    using namespace LegacyInformationTransition;
    assert(modelLightByte(0x7c0)==248);
    assert(leadModelLightByte(0x7c0)==124);
    assert(modelLightByte(0x400)==128);
    assert(leadModelLightByte(0x400)==64);
    assert(leadModelLightByte(-1)==0);
    return 0;
}

#include "game/legacy_information_transition.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
    float p=31.4f;
    p=LegacyInformationTransition::advanceXonixPhase(p,100);
    assert(p>31.8f); // no 10*pi wrap
    const float q=LegacyInformationTransition::advanceXonixPhase(0.f,20);
    assert(std::fabs(q-0.0999999978f)<1e-6f);
    std::cout<<"information xonix phase r337 PASS\n";
}

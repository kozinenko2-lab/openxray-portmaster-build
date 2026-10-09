#include "game/legacy_information_transition.hpp"
#include <cassert>
#include <iostream>
int main(){
    float p=6.2f;
    p=LegacyInformationTransition::advanceSwayPhase(p,100);
    assert(p>6.28f); // original does not wrap this phase
    std::cout<<"information sway phase r338 PASS\n";
}

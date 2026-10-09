#include "game/legacy_information_transition.hpp"
#include <cassert>
#include <iostream>
int main(){
 using namespace LegacyInformationTransition;
 int p=advancePromptPhase(0,123); assert(p==246);
 // Each original Information routine allocates a fresh zero phase on entry.
 p=0; assert(promptRedByte(p)==190);
 std::cout<<"information prompt reset r331 PASS\n";
}

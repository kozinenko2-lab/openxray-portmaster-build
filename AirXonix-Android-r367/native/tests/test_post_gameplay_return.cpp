#include "game/legacy_menu.hpp"
#include <cassert>
#include <iostream>
int main(){
    assert(kLegacyPostGameplayReturn.gameplayRoutine==0x004195D0u);
    assert(kLegacyPostGameplayReturn.callerReturnSite==0x00424EF1u);
    assert(kLegacyPostGameplayReturn.outerDispatchStart==0x00424EF9u);
    assert(kLegacyPostGameplayReturn.postDispatchConstructorCall==0x00424F3Bu);
    assert(kLegacyPostGameplayReturn.postGameplayConstructor==0x00423C30u);
    assert(kLegacyPostGameplayReturn.dispatchEnd==0x00424F50u);
    std::cout << "post-gameplay caller-owned return path ok\n";
}

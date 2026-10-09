#include "game/legacy_menu.hpp"
#include <cassert>
#include <iostream>
int main(){
    using T=LegacyGameplayCampaignTrace;
    static_assert(T::registrationFlag==0x025459B0u);
    static_assert(T::unregisteredModeSelector==0x004112F0u);
    static_assert(T::registeredModeSelector==0x00411870u);
    std::cout<<"mode select license split r335 PASS\n";
}

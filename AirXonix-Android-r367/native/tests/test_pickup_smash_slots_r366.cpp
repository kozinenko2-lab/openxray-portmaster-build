#include <cassert>
#include "game/legacy_effects.hpp"
int main(){
    using T=LegacyPickupSmashRuntimeTrace;
    static_assert(T::slotCount==6);
    static_assert(T::recordsPerSlot==128);
    static_assert(T::levelResetAgeMs==5000);
    assert(!T::activeAtFrameStart(5000));
    assert(T::activeAtFrameStart(0));
    assert(T::activeAtFrameStart(2499));
    assert(!T::activeAtFrameStart(2500));
    // 0x418110 checks before increment: this frame is still submitted.
    assert(T::steppedAge(2490,20)==2510);
    // Next frame is skipped and the age no longer advances.
    assert(T::steppedAge(2510,20)==2510);
}

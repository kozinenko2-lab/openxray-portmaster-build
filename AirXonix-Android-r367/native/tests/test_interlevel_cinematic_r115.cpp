#include "game/legacy_interlevel_trace.hpp"
#include "game/game.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

int main(){
    using T=LegacyInterLevel::Trace;
    static_assert(T::cinematicPhaseSeed==0x0041AA55u);
    static_assert(T::cinematicPairBegin==0x0041AE13u);
    static_assert(T::cinematicSlot5Submit==0x0041AE59u);
    static_assert(T::cinematicSlot0Submit==0x0041AEACu);
    static_assert(T::cinematicSlot5Model==0x0257F5B4u);
    static_assert(T::cinematicSlot5Prepared==0x0257F5E0u);
    static_assert(T::cinematicSlot0Model==0x0257F5A0u);
    static_assert(T::cinematicSlot0Prepared==0x0257F5CCu);
    static_assert(T::cinematicPhaseSeedMask==7);

    assert(std::fabs(T::cinematicPhasePerMs-0.002000000095f)<1e-9f);
    assert(std::fabs(T::cinematicSlot5Z+T::cinematicSlot0Z)<1e-9f);

    InterLevelState s{};
    s.cinematicPhase=0.f;
    assert(std::fabs(s.cinematicRotX()-T::cinematicRotXAmplitude)<1e-6f);
    assert(std::fabs(s.cinematicRotZ())<1e-6f);

    s.cinematicPhase=1.f;
    const float expectedX=std::cos(1.f)*T::cinematicRotXAmplitude;
    const float expectedZ=std::sin(T::cinematicRotZPhaseScale)*T::cinematicRotZAmplitude;
    assert(std::fabs(s.cinematicRotX()-expectedX)<1e-6f);
    assert(std::fabs(s.cinematicRotZ()-expectedZ)<1e-6f);

    std::cout << "interlevel cinematic r115 ok\n";
}

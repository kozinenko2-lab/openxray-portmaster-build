#include "audio/legacy_accumulator_ring.hpp"
#include <cassert>
using namespace airxonix;
int main(){
    static_assert(LegacyAccumulatorRingTrace::accumulatorBytes==0xAC440u);
    static_assert(LegacyAccumulatorRingTrace::outputBytes==0x2B110u);
    static_assert(LegacyAccumulatorRingTrace::fallbackLeadBytes==0x1800u);
    assert(LegacyAccumulatorRingTrace::advanceAccumulatorBytes(0,4)==4);
    assert(LegacyAccumulatorRingTrace::advanceAccumulatorBytes(0xAC43Cu,4)==0);
    assert(LegacyAccumulatorRingTrace::advanceOutput(0,1)==4);
    assert(LegacyAccumulatorRingTrace::advanceOutput(0xAC438u,2)==0);
}

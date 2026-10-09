#include "audio/legacy_output_quantizer.hpp"
#include <cassert>
using namespace airxonix;
int main(){
    static_assert(LegacyOutputQuantizerTrace::routine==0x0040AA80u);
    static_assert(LegacyOutputQuantizerTrace::maskTable==0x0043FCA8u);
    static_assert(LegacyOutputQuantizerTrace::addTable==0x0043FCACu);
    assert(LegacyOutputQuantizerTrace::quantizeAccumulator(-1)==0);
    assert(LegacyOutputQuantizerTrace::quantizeAccumulator(0)==0);
    assert(LegacyOutputQuantizerTrace::quantizeAccumulator(1)==1);
    assert(LegacyOutputQuantizerTrace::quantizeAccumulator(254)==254);
    assert(LegacyOutputQuantizerTrace::quantizeAccumulator(255)==255);
    assert(LegacyOutputQuantizerTrace::quantizeAccumulator(256)==255);
    assert(LegacyOutputQuantizerTrace::quantizeCenteredSum(-129)==0);
    assert(LegacyOutputQuantizerTrace::quantizeCenteredSum(-128)==0);
    assert(LegacyOutputQuantizerTrace::quantizeCenteredSum(-127)==1);
    assert(LegacyOutputQuantizerTrace::quantizeCenteredSum(0)==128);
    assert(LegacyOutputQuantizerTrace::quantizeCenteredSum(127)==255);
    assert(LegacyOutputQuantizerTrace::quantizeCenteredSum(128)==255);
}

#include "render/legacy_quality_trace.hpp"
#include <cassert>
#include <iostream>

int main(){
    static_assert(LegacyQualityTrace::globalAddress==0x0043F0ACu);
    static_assert(LegacyQualityTrace::successSetsOne==0x00407568u);
    static_assert(LegacyQualityTrace::onlyWrite==0x0040780Bu);
    static_assert(LegacyQualityTrace::initializedValue==1);
    static_assert(LegacyQualityTrace::highQuality);
    assert(LegacyQualityTrace::highQuality);
    std::cout << "quality global r249 PASS\n";
}

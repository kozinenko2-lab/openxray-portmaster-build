#include <cassert>
#include <iostream>
#include "game/legacy_dt_trace.hpp"
int main(){
    const auto&t=kLegacyScaledDtTrace;
    assert(t.wrapper==0x004194A0u && t.smoother==0x00405F90u && t.rawSampler==0x00405FD0u);
    assert(t.scaleAddress==0x0257D9E8u && t.outputAddress==0x0257DA7Cu);
    assert(t.minimumRawSampleMs==6 && t.movingAverageSamples==4 && t.maximumAverageMs==100);
    std::cout << "scaled dt r125 ok\n";
}

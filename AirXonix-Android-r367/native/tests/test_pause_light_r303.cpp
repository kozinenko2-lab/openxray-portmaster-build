#include "game/legacy_pause_trace.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
    const auto& t=kLegacyPauseTrace;
    assert(std::fabs(t.lightGray(t.enterStartY)-255.0000036f)<0.001f);
    assert(std::fabs(t.lightGray(t.holdY)-174.0f)<0.001f);
    assert(std::fabs(t.lightGray(t.exitY)-240.0f)<0.001f);
    std::cout<<"pause light r303 PASS\n";
}

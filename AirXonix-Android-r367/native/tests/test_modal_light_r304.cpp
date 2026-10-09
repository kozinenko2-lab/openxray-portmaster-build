#include "game/legacy_pause_trace.hpp"
#include "game/legacy_restore_trace.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
    // PAUS: RGB=(gray,gray,255), gray=165-y*300.
    assert(std::fabs(kLegacyPauseTrace.lightGray(kLegacyPauseTrace.holdY)-174.f)<0.001f);
    // ABOR normal/No: RGB=(gray,255,gray).
    assert(std::fabs(LegacyAbortConfirm.normalGray(-0.03f)-174.f)<0.001f);
    // ABOR Yes exit curve at hold position: outer=139, green=229.5.
    assert(std::fabs(LegacyAbortConfirm.yesOuter(-0.03f)-139.f)<0.002f);
    assert(std::fabs(LegacyAbortConfirm.yesGreen(-0.03f)-229.5f)<0.002f);
    std::cout<<"modal light r304 PASS\n";
}

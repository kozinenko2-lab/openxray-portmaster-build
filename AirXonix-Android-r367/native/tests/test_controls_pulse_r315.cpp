#include <cassert>
#include <cmath>
#include <iostream>
#include "game/legacy_controls_trace.hpp"
int main(){
 float p=0.f;p=kLegacyControlsTrace.advancePulse(p,50);
 assert(std::fabs(p-0.5f)<1e-5f);
 const int g=kLegacyControlsTrace.currentRowPulse(0.f);assert(g==255);assert(kLegacyControlsTrace.grayRgb(g)==0xffffffu);
 const int v=kLegacyControlsTrace.promptPulse(0.f);assert(v==160);assert(kLegacyControlsTrace.promptRgb(v)==0x82a082u);
 assert(kLegacyControlsTrace.currentRowPulse(3.14159265358979323846f)==75);
 std::cout<<"controls pulse r315 PASS\
";
}

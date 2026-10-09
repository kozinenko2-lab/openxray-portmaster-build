#include "game/legacy_records_transition.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
    using namespace LegacyRecordsTransition;
    // A phase with a fractional colour value must truncate, matching 0x43129C.
    for(int p: {1,17,123,377,901,1301}){
        constexpr double twoPi=6.283185307179586476925286766559;
        const double c=std::cos(double(p&0x7ff)*twoPi/2048.0);
        const int exact=std::clamp(static_cast<int>((c+1.0)*63.0+128.0),0,255);
        assert(namePulseByte(p)==exact);
    }
    std::cout << "records pulse r280 PASS\n";
}

#include "game/legacy_finale_entry_trace.hpp"
#include <cmath>
#include <iostream>
int main(){
    using T=LegacyFinaleEntry::Trace;
    static_assert(T::inputArmMs==7000);
    static_assert(T::autoExitMs==90000);
    if(std::fabs(T::presentationAngleInitial+0.12f)>1e-6f) return 1;
    if(std::fabs(T::presentationAngleCeiling+0.03f)>1e-6f) return 2;
    if(std::fabs(T::presentationYMax-0.1f)>1e-6f) return 3;
    if(std::fabs(T::sceneLightInitial-255.f)>1e-6f) return 4;
    if(std::fabs(T::sceneLightHold-200.f)>1e-6f) return 5;
    // Deterministic native interpretation: 255->200 at 0.05/ms = 1100ms,
    // then 200->0 at 0.1/ms = 2000ms once exit is requested.
    if(std::lround((T::sceneLightInitial-T::sceneLightHold)/T::sceneLightIntroFadePerMs)!=1100) return 6;
    if(std::lround(T::sceneLightHold/T::sceneLightExitFadePerMs)!=2000) return 7;
    std::cout << "r189 finale-entry trace PASS\n";
    return 0;
}

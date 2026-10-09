#include "render/legacy_main_menu_decor.hpp"
#include <cassert>
#include <cmath>
int main(){
    using T=LegacyMainMenuDecorationTrace;
    assert((T::menuPickupTypes==std::array<int,4>{{5,1,3,2}}));
    assert(std::fabs(T::menuPickupReflections[0]-.95f)<1e-7f);
    assert(std::fabs(T::menuPickupReflections[1]-.99f)<1e-7f);
    assert(std::fabs(T::menuPickupReflections[2]-.35f)<1e-7f);
    assert(std::fabs(T::menuPickupReflections[3]-.15f)<1e-7f);
    assert(std::fabs(T::menuPickupDirectional-.5f)<1e-7f && std::fabs(T::menuPickupAmbient-.2f)<1e-7f);
    assert(std::fabs(T::menuPostPickupDirectional-.7f)<1e-7f && std::fabs(T::menuPostPickupAmbient-.3f)<1e-7f);
    assert(T::pickup5TiltX==500);
    const float c=.25f;
    const float t0=T::menuPickupPhaseOffsets[0]-3.f*c;
    const float t1=T::menuPickupPhaseOffsets[1]-3.f*c;
    assert(std::fabs(T::orbitX(t0)-std::cos(t0)*.14f)<1e-6f);
    assert(std::fabs(T::orbitZ(t1)-(std::sin(t1)*.07f-.15f))<1e-6f);
}

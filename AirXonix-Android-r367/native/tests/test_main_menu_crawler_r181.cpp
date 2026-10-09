#include "render/legacy_main_menu_decor.hpp"
#include "render/legacy_transform.hpp"
#include <cassert>
#include <cmath>

static bool near(float a,float b,float e=1e-6f){return std::fabs(a-b)<=e;}

int main(){
    static_assert(LegacyMainMenuDecorationTrace::crawlerPrimaryReflection > 0.299f &&
                  LegacyMainMenuDecorationTrace::crawlerPrimaryReflection < 0.301f);
    static_assert(LegacyMainMenuDecorationTrace::crawlerSecondaryReflection == 0.5f);

    for(const auto [pitch,yaw] : {std::pair<float,float>{0.f,0.f},{0.2f,0.4f},{-0.31f,0.77f}}){
        const auto p=LegacyMainMenuDecorationTrace::crawlerPrimaryPosition(pitch,yaw);
        auto m=LegacyTransform::identity();
        LegacyTransform::rotateXRad(m,pitch);
        LegacyTransform::rotateYRad(m,yaw);
        const auto q=LegacyTransform::transformPoint(m,
            LegacyMainMenuDecorationTrace::crawlerPrimaryOrbitRadius,
            LegacyMainMenuDecorationTrace::crawlerPrimaryLiftRadius,0.f);
        assert(near(p.x,q.x)); assert(near(p.y,q.y)); assert(near(p.z,q.z));
    }
    const auto zero=LegacyMainMenuDecorationTrace::crawlerPrimaryPosition(0.f,0.f);
    assert(near(zero.x,0.11f)); assert(near(zero.y,0.07f)); assert(near(zero.z,0.f));
    return 0;
}

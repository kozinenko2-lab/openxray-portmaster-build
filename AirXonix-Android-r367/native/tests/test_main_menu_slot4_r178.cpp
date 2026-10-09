#include "render/legacy_main_menu_decor.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

static bool nearf(float a,float b,float eps=2e-6f){return std::fabs(a-b)<=eps;}
int main(){
    using T=LegacyMainMenuDecorationTrace;
    // With no shared wave and phase 0, the two local points are opposite ends
    // of the .04 orbit around (0.11,.05,0): z=+-.04, x=.11, y=.05.
    auto a=T::slot4Position(0,0.f,0.f,false);
    auto b=T::slot4Position(0,0.f,0.f,true);
    assert(nearf(a.x,T::slot4CenterX));
    assert(nearf(a.y,T::slot4CenterY));
    assert(nearf(a.z,T::slot4Radius));
    assert(nearf(b.x,T::slot4CenterX));
    assert(nearf(b.y,T::slot4CenterY));
    assert(nearf(b.z,-T::slot4Radius));

    // At quarter turn, local x is center+radius and local z is zero.
    auto q=T::slot4Position(512,0.f,0.f,false);
    assert(nearf(q.x,T::slot4CenterX+T::slot4Radius));
    assert(nearf(q.y,T::slot4CenterY));
    assert(nearf(q.z,0.f));

    // The pair remains diametrically opposite around the transformed centre.
    const float pitch=.23f,yaw=-.41f;
    auto p0=T::slot4Position(137,pitch,yaw,false);
    auto p1=T::slot4Position(137,pitch,yaw,true);
    auto c0=T::slot4Position(0,pitch,yaw,false);
    auto c1=T::slot4Position(0,pitch,yaw,true);
    const float centerX=(c0.x+c1.x)*.5f, centerY=(c0.y+c1.y)*.5f, centerZ=(c0.z+c1.z)*.5f;
    assert(nearf((p0.x+p1.x)*.5f,centerX));
    assert(nearf((p0.y+p1.y)*.5f,centerY));
    assert(nearf((p0.z+p1.z)*.5f,centerZ));
    std::cout << "main menu slot4 r178 ok\n";
}

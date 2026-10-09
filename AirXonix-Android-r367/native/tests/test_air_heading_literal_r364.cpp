#include "game/entities.hpp"
#include <iostream>

struct EntitiesTestProbeR364 {
    static int heading(float vx,float vy){return Entities::legacyAirHeading2048(vx,vy);}
};
static bool check(bool v,const char* m){if(!v)std::cerr<<"FAIL: "<<m<<"\n";return v;}
int main(){
    bool ok=true;
    // Quadrant IV is the distinguishing case: native stores a negative angle.
    const int q4=EntitiesTestProbeR364::heading(1.f,-1.f);
    ok &= check(q4<0,"quadrant-IV native heading must remain negative before matrix masking");
    ok &= check(q4==-255 || q4==-256,"-pi/4 must convert near -256 legacy units");
    // Quadrant II: atan(vy/vx) is negative, then native adds PI.
    const int q2=EntitiesTestProbeR364::heading(-1.f,1.f);
    ok &= check(q2==767 || q2==768,"quadrant-II heading must be near +768 units");
    // Quadrant III: positive ratio + PI gives an angle >PI, not a signed wrap.
    const int q3=EntitiesTestProbeR364::heading(-1.f,-1.f);
    ok &= check(q3==1279 || q3==1280,"quadrant-III heading must be near +1280 units");
    if(!ok)return 1;
    std::cout<<"air heading literal r364 ok\n";
}

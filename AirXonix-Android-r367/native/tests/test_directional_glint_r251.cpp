#include "render/legacy_directional_glint.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

static bool near(float a,float b,float e=2e-6f){return std::fabs(a-b)<=e;}
static void checkRadius(const std::array<LegacyDirectionalGlint::Vertex,7>& v,
                        float x,float y,float z,float radius){
    for(const auto& q:v){
        const float dx=q.x-x,dy=q.y-y,dz=q.z-z;
        assert(near(std::sqrt(dx*dx+dy*dy+dz*dz),radius));
    }
}
int main(){
    using namespace LegacyDirectionalGlint;
    static_assert(Trace::airborneSize==0.005499999970197678f);
    static_assert(Trace::airborneY==0.004999999888241291f);
    static_assert(Trace::crawlerSize==0.0035000001080334187f);
    static_assert(Trace::crawlerYOffset==0.013000000268220901f);

    const auto air=build(.55f,.10f,.35f,.48f,Trace::airborneY,.42f,Trace::airborneSize);
    checkRadius(air,.48f,Trace::airborneY,.42f,Trace::airborneSize);
    const float crawlerPhase=.021f;
    const auto crawler=build(.55f,.10f,.35f,.44f,crawlerPhase+Trace::crawlerYOffset,.46f,Trace::crawlerSize);
    checkRadius(crawler,.44f,crawlerPhase+Trace::crawlerYOffset,.46f,Trace::crawlerSize);
    std::cout << "directional glint r251 PASS\n";
}

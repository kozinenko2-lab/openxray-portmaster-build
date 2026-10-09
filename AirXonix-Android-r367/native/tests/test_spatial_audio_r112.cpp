#include "audio/legacy_spatial_audio.hpp"
#include <cassert>
using namespace airxonix;
int main(){
    static_assert(LegacySpatialAudioTrace::channelClamp==85);
    auto id=legacyBuildAudioBasis(0,0,0);assert(id.a[0]<-0.999f&&id.b[0]>0.999f);
    LegacySpatialListener l{};l.basis.a={{1,0,0}};l.basis.b={{-1,0,0}};
    auto g=legacySpatialGains(l,1,0,0,1.f);
    assert(g.left==6); // round(2.56*2.5)
    assert(g.right==1); // round(2.56*0.5)
    auto far=legacySpatialGains(l,100,0,0,1.f);assert(far.left==0||far.left==1);
    auto loud=legacySpatialGains(l,.01f,0,0,10.f);assert(loud.left==85&&loud.right==85);
}

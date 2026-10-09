#include "audio/legacy_spatial_audio.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
using namespace airxonix;

static bool near(float a,float b,float e=1e-6f){return std::fabs(a-b)<=e;}

int main(){
    using T=LegacySpatialAudioTrace;
    static_assert(T::routine==0x0040A7B0u);
    static_assert(T::bootstrapRoutine==0x0040A3D0u);
    static_assert(T::basisRoutine==0x0040A450u);
    static_assert(T::listenerRoutine==0x0040A5C0u);
    static_assert(T::channelClamp==0x55);
    static_assert(T::bootstrapSourceScale==0.1f);
    static_assert(T::bootstrapBias==1.5f);
    static_assert(T::bootstrapDistanceScale==25.6f);
    static_assert(T::combinedScale==2.56f);

    // 0x418D1E..0x418DA4 and 0x424D19..0x424D28 call 0x40A3D0
    // with (pi, 1.5f, 0.1f), hence 64/(1.5+1)*0.1 = 2.56.
    assert(near((64.0f/(T::bootstrapBias+1.0f))*T::bootstrapSourceScale,T::combinedScale));

    // 0x40A7B0 normalizes source-listener delta, then computes two x87 FISTP
    // gains: scalar*2.56/distance * (1.5 + dot(direction,basisChannel)).
    LegacySpatialListener l{};
    l.basis.a={{1.f,0.f,0.f}};
    l.basis.b={{-1.f,0.f,0.f}};
    auto g=legacySpatialGains(l,1.f,0.f,0.f,1.f);
    assert(g.left==6);   // lrint(2.56 * 2.5)
    assert(g.right==1);  // lrint(2.56 * 0.5)

    auto symmetric=legacySpatialGains(l,0.f,0.f,1.f,1.f);
    assert(symmetric.left==4 && symmetric.right==4); // lrint(2.56*1.5)

    // Very near sources saturate at the literal unsigned clamp 0x55.
    auto saturated=legacySpatialGains(l,0.001f,0.f,0.f,1.f);
    assert(saturated.left==85 && saturated.right==85);

    // The actual game bootstrap basis (0,-302,0) remains a unit pair.
    auto b=legacyBuildAudioBasis(0,-302,0);
    auto len=[](const std::array<float,3>& v){return std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);};
    assert(std::fabs(len(b.a)-1.f)<1e-5f);
    assert(std::fabs(len(b.b)-1.f)<1e-5f);
    assert(std::fabs(b.a[0]+b.b[0])<1e-5f);
    assert(std::fabs(b.a[1]+b.b[1])<1e-5f);
    assert(std::fabs(b.a[2]+b.b[2])<1e-5f);
}

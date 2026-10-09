#include "legacy_settings_widgets.hpp"
#include "legacy_mesh_builder.hpp"
#include <cmath>
#include <vector>

namespace LegacySettingsWidgets {
namespace {
constexpr float kHalfPi=1.5707963705062866f;
}

LegacyMesh buildTrack(){
    // Literal 10-point profile written at 0x422B9B..0x422C20.
    const std::vector<LegacyProfilePoint> profile={
        {0.0010000000474974513f,  0.5f},
        {0.05000000074505806f,   0.44999998807907104f},
        {0.05000000074505806f,   0.4399999976158142f},
        {0.02500000037252903f,   0.4300000071525574f},
        {0.02500000037252903f,   0.41999998688697815f},
        {0.02500000037252903f,  -0.41999998688697815f},
        {0.02500000037252903f,  -0.4300000071525574f},
        {0.05000000074505806f,  -0.4399999976158142f},
        {0.05000000074505806f,  -0.44999998807907104f},
        {0.0010000000474974513f,-0.5f}
    };
    LegacyMeshBuilder b;
    b.addRevolvedProfile(profile,+kHalfPi,-kHalfPi,16,
                         0.001953125f,0.251953125f,
                         0.001953125f,0.251953125f,
                         0.08000000566244125f);
    return b.take();
}

LegacyMesh buildKnob(){
    // 0x42154E..0x421579 creates eight profile samples:
    // angle = pi/2 - i*0.4430846869945526 - 0.02,
    // profile=(cos(angle),sin(angle)). Hardware path index 3 then sends this
    // profile to 0x402A50 at 0x421650..0x42167B.
    std::vector<LegacyProfilePoint> profile;
    profile.reserve(8);
    constexpr float step=0.4430846869945526f;
    constexpr float bias=0.019999999552965164f;
    for(int i=0;i<8;++i){
        const float a=kHalfPi-float(i)*step-bias;
        profile.push_back({std::cos(a),std::sin(a)});
    }
    LegacyMeshBuilder b;
    b.addRevolvedProfile(profile,+kHalfPi,-kHalfPi,16,
                         0.064453125f,0.123046875f,
                         0.064453125f,0.123046875f,
                         0.004999999888241291f);
    return b.take();
}

} // namespace LegacySettingsWidgets

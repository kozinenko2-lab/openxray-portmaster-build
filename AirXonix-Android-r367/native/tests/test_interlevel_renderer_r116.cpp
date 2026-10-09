#include "render/legacy_model_factory.hpp"
#include "render/legacy_reflection.hpp"
#include "game/legacy_interlevel_trace.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

int main(){
    using T=LegacyInterLevel::Trace;
    static_assert(T::cinematicTextureSelect==0x0041ADCFu);
    static_assert(T::cinematicTextureSlot==4);
    const auto slot0=LegacyModelFactory::buildCinematicSlot(0);
    const auto slot5=LegacyModelFactory::buildCinematicSlot(5);
    assert(slot0.vertices.size()==32u);
    assert(slot0.faces.size()==14u);
    assert(slot5.vertices.size()==8u);
    assert(slot5.faces.size()==3u);
    float minU0=99,maxU0=-99,minV0=99,maxV0=-99;
    for(const auto& v:slot0.vertices){minU0=std::min(minU0,v.u);maxU0=std::max(maxU0,v.u);minV0=std::min(minV0,v.v);maxV0=std::max(maxV0,v.v);}
    assert(std::fabs(minU0-0.001953125f)<1e-6f);
    assert(std::fabs(maxU0-0.998046875f)<1e-6f);
    assert(std::fabs(minV0-0.19921875f)<1e-6f);
    assert(std::fabs(maxV0-0.373046875f)<1e-6f);
    float minU5=99,maxU5=-99,minV5=99,maxV5=-99;
    for(const auto& v:slot5.vertices){minU5=std::min(minU5,v.u);maxU5=std::max(maxU5,v.u);minV5=std::min(minV5,v.v);maxV5=std::max(maxV5,v.v);}
    assert(std::fabs(minU5-0.001953125f)<1e-6f);
    assert(std::fabs(maxU5-0.498046875f)<1e-6f);
    assert(std::fabs(minV5-0.501953125f)<1e-6f);
    assert(std::fabs(maxV5-0.685546875f)<1e-6f);
    auto m=LegacyTransform::identity();
    LegacyReflection::rotateX(m,0.27f);
    LegacyReflection::rotateZ(m,0.21f);
    // Orthogonal unit axes remain unit length under the exact radian rotations.
    const float lx=std::sqrt(m.m[0]*m.m[0]+m.m[1]*m.m[1]+m.m[2]*m.m[2]);
    assert(std::fabs(lx-1.f)<1e-5f);
    std::cout << "interlevel renderer r116 ok\n";
}

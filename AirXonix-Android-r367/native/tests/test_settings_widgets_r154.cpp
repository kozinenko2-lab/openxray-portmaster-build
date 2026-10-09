#include "render/legacy_settings_widgets.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

static bool near(float a,float b,float e=1e-6f){return std::fabs(a-b)<=e;}
int main(){
    const auto& t=LegacySettingsWidgets::kTrace;
    assert(t.trackConstructor==0x00422B6Bu && t.trackModel==0x0257F584u);
    assert(t.knobConstructor==0x00421647u && t.knobModel==0x0257F580u);
    assert(t.knobPrepared==0x02583754u && t.radialSegments==16);
    const auto track=LegacySettingsWidgets::buildTrack();
    const auto knob=LegacySettingsWidgets::buildKnob();
    // addRevolvedProfile emits (radial+1)*profile points and radial*(P-1) quads.
    assert(track.vertices.size()==17u*10u);
    assert(track.faces.size()==16u*9u);
    assert(knob.vertices.size()==17u*8u);
    assert(knob.faces.size()==16u*7u);
    assert(near(track.vertices.front().u,0.001953125f));
    assert(near(track.vertices.back().u,0.251953125f));
    assert(near(knob.vertices.front().u,0.064453125f));
    assert(near(knob.vertices.back().u,0.123046875f));
    // Profile endpoint Y values after the literal constructor scale.
    assert(near(track.vertices.front().y,0.0400000028f,2e-6f));
    assert(knob.vertices.front().y>0.0049f && knob.vertices.front().y<0.0051f);
    std::cout << "r154 exact Settings track/knob meshes ok\n";
}

#include "game/legacy_effects.hpp"
#include "render/legacy_model_factory.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
    for(int i=0;i<6;++i){
        auto m=LegacyModelFactory::buildAuxiliarySlot(i);
        assert(!m.vertices.empty() && !m.faces.empty());
    }
    const float p=legacyAuxiliaryPitch(.103f,.4f,.03f,.4f);
    assert(std::isfinite(p));
    assert(std::fabs(p+3.1415927f)<1e-4f); // atan(+inf)=+pi/2
    std::cout<<"auxiliary render r339 PASS\n";
}

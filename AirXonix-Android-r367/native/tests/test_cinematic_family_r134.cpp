#include <cassert>
#include "render/legacy_model_factory.hpp"
#include "render/legacy_cinematic_family_trace.hpp"
int main(){
    using namespace LegacyCinematicFamily;
    for(const auto& s:Slots){
        const auto m=LegacyModelFactory::buildCinematicSlot(s.index);
        assert(!m.vertices.empty());
        assert(!m.faces.empty());
    }
    static_assert(Slots[1].prepared==0x0257F5D0u);
    static_assert(Slots[2].prepared==0x0257F5D4u);
}

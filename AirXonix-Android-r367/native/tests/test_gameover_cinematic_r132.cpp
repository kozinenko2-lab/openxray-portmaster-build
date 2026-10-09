#include <cassert>
#include "render/legacy_model_factory.hpp"
#include "render/legacy_gameover_trace.hpp"
int main(){
    using namespace LegacyGameOver;
    static_assert(Trace::cinematicSlot==3);
    static_assert(Trace::preparedPointer==0x0257F5D8u);
    static_assert(Trace::textureSlot==4);
    const auto m=LegacyModelFactory::buildCinematicSlot(3);
    assert(!m.vertices.empty());
    assert(!m.faces.empty());
    assert(Trace::submitZ==0.003000000026077032f);
}

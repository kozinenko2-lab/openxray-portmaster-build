#include <cassert>
#include "render/legacy_model_factory.hpp"
#include "render/legacy_abort_trace.hpp"
int main(){
    using namespace LegacyAbort;
    static_assert(Trace::cinematicSlot==4);
    static_assert(Trace::preparedPointer==0x0257F5DCu);
    static_assert(Trace::textureSlot==4);
    const auto m=LegacyModelFactory::buildCinematicSlot(4);
    assert(!m.vertices.empty());
    assert(!m.faces.empty());
    assert(Trace::panelYScale==1.100000023841858f);
}

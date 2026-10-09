#include "render/legacy_background_scroll.hpp"
#include "render/legacy_field_frontend.hpp"
#include "game/game.hpp"
#include <cassert>
#include <cmath>

static bool near(float a,float b,float e=2e-6f){return std::fabs(a-b)<=e;}

int main(){
    using namespace LegacyBackgroundScroll;
    assert(near(advance(0.f,1000),.1f));
    assert(near(advance(.9995f,10),.0005f));

    const auto g=LegacyFieldFrontend::build(.5f,.1f,.5f);
    const auto r=LegacyFieldFrontend::backgroundRing(g,.25f);
    assert(near(r[0].v,.25f));
    assert(near(r[1].v,FarV+.25f));
    assert(near(r[2].v,FarV+.25f));
    assert(near(r[3].v,.25f));
    assert(near(r[4].v,g.outerMinZ*LegacyFieldFrontend::BackgroundUvScale+.25f));
    assert(near(r[6].v,g.outerMaxZ*LegacyFieldFrontend::BackgroundUvScale+.25f));

    // 0x419FE8 calls 0x41FD50 once in an ordinary gameplay frame.
    Game game;
    const float before=game.legacyBackgroundVPhase();
    InputState idle{};
    game.update(idle,16);
    assert(near(game.legacyBackgroundVPhase(),advance(before,16)));
    return 0;
}

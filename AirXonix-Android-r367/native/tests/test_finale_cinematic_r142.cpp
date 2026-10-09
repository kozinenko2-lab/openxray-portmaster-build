#include <cassert>
#include <cmath>
#include "game/game.hpp"
#include "game/legacy_finale_cinematic_trace.hpp"
struct GameTestProbe { static void finale(Game& g){ g.levelIntroScene_.elapsedMs=g.levelIntroScene_.durationMs; g.levelEntryScene_.running=false; g.beginFinalSequence(); } };
int main(){
    using T=LegacyFinaleCinematic::Trace;
    static_assert(T::phaseSeed==0x0041B468u);
    static_assert(T::slot5Submit==0x0041BA28u);
    static_assert(T::slot0Submit==0x0041BA3Bu);
    static_assert(T::textureSlot==4);
    static_assert(T::slot5Z==0.f);
    static_assert(T::slot0Z<-.0079f && T::slot0Z>-.0081f);
    Game g;
    GameTestProbe::finale(g);
    const float p0=g.finalScene().cinematicPhase;
    assert(p0>=0.f && p0<=7.f);
    g.update(InputState{},100);
    const float expected=p0+100.f*T::phasePerMs;
    assert(std::fabs(g.finalScene().cinematicPhase-expected)<1e-5f);
    assert(std::fabs(g.finalScene().cinematicRotX()-std::cos(expected)*T::rotXAmplitude)<1e-5f);
    assert(std::fabs(g.finalScene().cinematicRotZ()-std::sin(expected*T::rotZPhaseScale)*T::rotZAmplitude)<1e-5f);
    return 0;
}

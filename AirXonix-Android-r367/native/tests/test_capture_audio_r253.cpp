#include "game/game.hpp"
#include <cassert>
#include <iostream>

struct GameTestProbe {
    static Field& field(Game& g){ return g.field_; }
    static void consumeCaptureAudio(Game& g){ g.consumeCaptureStartedAudio(); }
};

int main(){
    Game g;
    (void)g.takeDeathAudioEvents();
    auto& f=GameTestProbe::field(g);
    LevelRecord level{};

    // A split field leaves a real marker component after preserving the left
    // side, so 0x418598/0x4185BB must issue simple SFX 6 (up01).
    f.build(level);
    for(int y=1;y<63;++y)f.set(32,y,Field::Safe);
    const auto marker=f.beginCapture({GridSeed{16,31}});
    assert(marker!=0 && f.markerCellCount(marker)>0);
    GameTestProbe::consumeCaptureAudio(g);
    auto ev=g.takeDeathAudioEvents();
    assert(ev.size()==1);
    assert(ev[0].kind==DeathAudioEventKind::SimplePlay);
    assert(ev[0].logicalId==0x06u);

    // The event is one-shot.
    GameTestProbe::consumeCaptureAudio(g);
    assert(g.takeDeathAudioEvents().empty());

    // If every would-be marker cell belongs to a preserved component, the EXE
    // leaves the tiny marker timer allocated but does NOT call 0x40AE50(6).
    f.build(level);
    const auto emptyMarker=f.beginCapture({GridSeed{31,31}});
    assert(emptyMarker!=0);
    assert(f.markerCellCount(emptyMarker)==0);
    GameTestProbe::consumeCaptureAudio(g);
    assert(g.takeDeathAudioEvents().empty());

    std::cout << "capture audio r253 ok\n";
    return 0;
}

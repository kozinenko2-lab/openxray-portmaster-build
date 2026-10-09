#include "game/game.hpp"
#include <cassert>
#include <iostream>

static int musicRequests(std::vector<DeathAudioEvent> ev){
    int n=0; for(const auto& e:ev) if(e.kind==DeathAudioEventKind::MusicRequest) ++n; return n;
}

int main(){
    Game g;
    // Constructor/loadLevel has selected the environment already, but the
    // gameplay music selector must wait until 0x41CEA0's intro has returned.
    assert(g.levelIntroActive());
    assert(musicRequests(g.takeDeathAudioEvents())==0);
    InputState in{};
    g.update(in,1000);
    assert(g.levelIntroActive());
    assert(musicRequests(g.takeDeathAudioEvents())==0);
    g.update(in,1000);
    assert(g.levelIntroActive());
    assert(musicRequests(g.takeDeathAudioEvents())==0);
    g.update(in,1000);
    assert(g.levelIntroActive());
    assert(musicRequests(g.takeDeathAudioEvents())==0);
    g.update(in,1000);
    assert(!g.levelIntroActive());
    assert(musicRequests(g.takeDeathAudioEvents())==1);
    g.update(in,16);
    assert(musicRequests(g.takeDeathAudioEvents())==0);
    std::cout << "level intro music order r291 PASS\n";
}

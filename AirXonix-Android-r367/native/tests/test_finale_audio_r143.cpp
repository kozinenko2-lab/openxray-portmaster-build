#include <cassert>
#include <cmath>
#include "game/game.hpp"
struct GameTestProbe { static void finale(Game& g){ g.beginFinalSequence(); } };
int main(){
    Game g;
    (void)g.takeDeathAudioEvents();
    GameTestProbe::finale(g);
    auto ev=g.takeDeathAudioEvents();
    bool comp=false,fade=false,track=false;
    for(const auto& e:ev){
        if(e.kind==DeathAudioEventKind::SimplePlay && e.logicalId==0x17u)comp=true;
        if(e.kind==DeathAudioEventKind::MusicFadeOut && std::fabs(e.fadePerMs-0.0010000000474974513f)<1e-9f)fade=true;
        if(e.kind==DeathAudioEventKind::MusicRequest && e.logicalId==7u && std::fabs(e.fadePerMs-0.0005000000237487257f)<1e-9f)track=true;
    }
    assert(comp&&fade&&track);
    return 0;
}

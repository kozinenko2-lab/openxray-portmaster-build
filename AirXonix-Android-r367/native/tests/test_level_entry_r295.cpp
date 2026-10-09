#include "game/game.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
static bool nearf(float a,float b,float e=2e-5f){return std::fabs(a-b)<=e;}
int main(){
    Game g;
    g.takeDeathAudioEvents();
    for(int i=0;i<4 && g.levelIntroActive();++i)g.update({},1000);
    assert(!g.levelIntroActive());
    assert(g.levelEntryActive());
    auto ev=g.takeDeathAudioEvents();
    bool start=false,music=false;
    for(const auto& e:ev){
        if(e.kind==DeathAudioEventKind::SpatialStart && e.voice==DeathAudioVoiceTag::LevelEntryVoice && e.logicalId==7)start=true;
        if(e.kind==DeathAudioEventKind::MusicRequest)music=true;
    }
    assert(start && music);
    const int timer0=g.timeRemaining();
    g.update({},100);
    assert(g.levelEntryActive());
    assert(g.timeRemaining()==timer0);
    assert(nearf(g.levelEntryScene().light,17.241378f,2e-3f));
    assert(g.levelEntryScene().cameraAngle2 < -302);
    assert(g.displayPlayerHeight() < 0.19f);
    // Reach full light, then one additional update performs the original top-of-loop exit.
    for(int i=0;i<20 && g.levelEntryScene().light<255.f;i++)g.update({},100);
    assert(g.levelEntryActive());
    assert(nearf(g.levelEntryScene().light,255.f,1e-4f));
    g.takeDeathAudioEvents();
    g.update({},1);
    assert(!g.levelEntryActive());
    assert(g.timeRemaining()==timer0);
    ev=g.takeDeathAudioEvents();
    bool stop=false,land=false;
    for(const auto& e:ev){
        if(e.kind==DeathAudioEventKind::SpatialStop && e.voice==DeathAudioVoiceTag::LevelEntryVoice)stop=true;
        if(e.kind==DeathAudioEventKind::SpatialPlay && e.logicalId==9)land=true;
    }
    assert(stop && land);
    g.update({},1);
    assert(g.timeRemaining()<timer0);
    std::cout<<"level entry r295 PASS\n";
}

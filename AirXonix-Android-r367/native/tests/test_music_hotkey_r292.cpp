#include "game/game.hpp"
#include <cassert>
#include <iostream>
int main(){
    Game g; InputState n{};
    g.update(n,1000); g.takeDeathAudioEvents();
    g.update(n,1000); g.takeDeathAudioEvents();
    g.update(n,1000); g.takeDeathAudioEvents();
    g.update(n,1000); g.takeDeathAudioEvents(); // r347 light-pulse exit + music request
    for(int i=0;i<20 && g.levelEntryActive();++i){g.update(n,100); g.takeDeathAudioEvents();}
    if(g.levelEntryActive()){g.update(n,1); g.takeDeathAudioEvents();}
    const int before=g.timeRemaining();
    InputState m{}; m.legacyPressedCode=0x4D; m.legacyPressedFromController=false;
    g.update(m,16);
    auto ev=g.takeDeathAudioEvents();
    int fades=0, req=0;
    for(const auto& e:ev){
        if(e.kind==DeathAudioEventKind::MusicFadeOut){ ++fades; assert(e.fadePerMs>0.00099f && e.fadePerMs<0.00101f); }
        if(e.kind==DeathAudioEventKind::MusicRequest){ ++req; assert(e.fadePerMs>0.00049f && e.fadePerMs<0.00051f); }
    }
    assert(fades==1 && req==1);
    assert(g.timeRemaining()==before-16); // hotkey does not consume/abort the frame
    InputState cm=m; cm.legacyPressedFromController=true;
    g.update(cm,16);
    ev=g.takeDeathAudioEvents(); fades=req=0;
    for(const auto& e:ev){ fades += e.kind==DeathAudioEventKind::MusicFadeOut; req += e.kind==DeathAudioEventKind::MusicRequest; }
    assert(fades==0 && req==0);
    std::cout << "music hotkey r292 PASS\n";
}

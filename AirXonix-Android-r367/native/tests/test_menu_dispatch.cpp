#include "game/game.hpp"
#include "game/legacy_menu.hpp"
#include <cassert>
#include <iostream>

struct GameTestProbe {
    static void enterMainMenu(Game& g){g.enterMainMenu();}
};

static void menuStep(Game& g,bool down){
    InputState press{}; if(down)press.down=true; else press.up=true;
    InputState release{};
    g.update(press,16);
    // r184: no new event is polled until selectorOffset reaches its row target.
    for(int i=0;i<8;++i)g.update(release,16);
}

int main(){
    static_assert(kLegacyMainMenuEntries.size()==5);
    assert(kLegacyMainMenuEntries[0].targetAddress==0x00424DE0u);
    assert(kLegacyMainMenuEntries[1].targetAddress==0x00413670u);
    assert(kLegacyMainMenuEntries[2].targetAddress==0x0040F160u);
    assert(kLegacyMainMenuEntries[3].targetAddress==0x00410CF0u);
    assert(kLegacyMainMenuEntries[4].kind==LegacyMenuDispatchKind::ReturnFromMenu);
    assert(kLegacyMainMenuEntries[4].targetAddress==0x00413662u);
    assert(kLegacyGameplayHudConstructor==0x00423350u);
    assert(kLegacyPostGameplayM1Constructor==0x00423C30u);
    assert(kLegacyEntry1M2Constructor==0x00423EE0u);

    Game g; GameTestProbe::enterMainMenu(g);
    InputState release{};
    for(int i=0;i<4;++i)menuStep(g,true);
    assert(g.mainMenu().selected==4);
    for(int i=0;i<4;++i)menuStep(g,false);
    assert(g.mainMenu().selected==0);
    InputState back{}; back.back=true; g.update(back,16); g.update(release,16);
    assert(g.mainMenu().selected==4);
    assert(!g.wantsQuit());
    InputState action{}; action.action=true; g.update(action,16);
    assert(g.mainMenu().hasPendingDispatch);
    assert(g.mainMenu().pendingDispatch.kind==LegacyMenuDispatchKind::ReturnFromMenu);
    std::cout<<"main-menu dispatch trace ok\n";
}

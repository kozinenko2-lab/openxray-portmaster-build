#include "game/game.hpp"
#include "platform/android_touch_menu.hpp"
#include <cassert>
#include <iostream>

struct GameTestProbe {
    static void enterMainMenu(Game& g) {g.enterMainMenu();}
};

int main(){
    Game g; GameTestProbe::enterMainMenu(g);
    const InputState released{};
    InputState down{};down.down=true;
    g.update(down,16);
    assert(g.mainMenu().selected==1);
    // Release WHILE the selector is still moving, then immediately flick
    // again when it stops. The old native latch silently ate the second flick.
    for(int i=0;i<8;++i)g.update(released,16);
    g.update(down,16);
    assert(g.mainMenu().selected==2);
    for(int i=0;i<8;++i)g.update(released,16);
    InputState up{};up.up=true;
    g.update(up,16);
    assert(g.mainMenu().selected==1);
    for(int i=0;i<8;++i)g.update(released,16);
    g.update(up,16);
    assert(g.mainMenu().selected==0);
    for(int i=0;i<8;++i)g.update(released,16);
    InputState action{};action.action=true;
    g.update(action,16);
    assert(g.phase()==GamePhase::ModeSelect);
    for(int i=0;i<65;++i)g.update(released,16); // complete fade > 0x7C0
    assert(g.modeSelect().readyForInput);
    assert(!g.modeSelect().inputLatched);
    g.update(down,16); // first touch in menu must not be lost
    assert(g.modeSelect().selected==1);
    std::cout << "Android main menu latch and first mode-select tap PASS\n";
}

#include "game/game.hpp"
#include <cassert>
#include <cstddef>

struct GameTestProbe {
    static void enterModeSelect(Game& g){ g.enterModeSelect(); }
};

int main(){
    for(std::size_t wanted=0; wanted<5; ++wanted){
        Game g;
        GameTestProbe::enterModeSelect(g);
        assert(g.phase()==GamePhase::ModeSelect);
        assert(g.modeSelect().selected==0);

        InputState release{};
        // r305: selector input is gated until the +3/ms fade reaches 0x7C0.
        for(int i=0;i<50 && !g.modeSelect().readyForInput;++i)g.update(release,16);
        g.update(release,16); // release the fresh-key latch
        for(std::size_t i=0;i<wanted;++i){
            InputState down{}; down.down=true;
            g.update(down,16);
            g.update(release,16);
        }
        assert(g.modeSelect().selected==static_cast<int>(wanted));

        InputState action{}; action.action=true;
        g.update(action,16);
        assert(g.phase()==GamePhase::ModeSelect);
        for(int i=0;i<40 && g.phase()==GamePhase::ModeSelect;++i)g.update(release,16);
        assert(g.phase()==GamePhase::Gameplay);
        assert(g.modeIndex()==wanted);
        assert(g.levelIndex()==0);
    }
    return 0;
}

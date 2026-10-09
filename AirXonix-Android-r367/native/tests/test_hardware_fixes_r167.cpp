#include <cassert>
#include <cmath>
#include "game/game.hpp"

struct GameTestProbe {
    static void settings(Game& g){ g.enterSettings(); }
    static void start(Game& g){ g.startNewSession(0); }
};

static void release(Game& g){ InputState n{}; g.update(n,16); }
static void settle(Game& g){ InputState n{}; for(int i=0;i<16 && std::fabs(g.settings().selectorOffset-LegacySettingsTrace::selectorTarget(g.settings().selected))>1e-7f;++i) g.update(n,16); }
static void down(Game& g){ InputState i{}; i.down=true; g.update(i,16); release(g); settle(g); }

int main(){
    Game g;
    GameTestProbe::settings(g);
    assert(g.settings().testInitialLives==3);
    assert(g.settings().testInitialTimeSeconds==60);
    // r309+: the original Settings screen ignores navigation until its 0..0x7C0 fade-in completes.
    while(!g.settings().readyForInput){ InputState n{}; g.update(n,100); }
    release(g);
    for(int i=0;i<6;++i) down(g);
    assert(g.settings().selected==6);
    InputState r{}; r.right=true; g.update(r,16); release(g);
    assert(g.settings().testInitialLives==4);
    down(g);
    assert(g.settings().selected==7);
    r={}; r.right=true; g.update(r,16); release(g);
    assert(g.settings().testInitialTimeSeconds==70);
    GameTestProbe::start(g);
    assert(g.lives()==4);
    assert(g.timeRemaining()==(70<<10));

    Player p;
    p.reset();
    p.prepareRespawnCoordinates();
    assert(p.x()==32 && p.y()==0 && p.prevX()==32 && p.prevY()==0);
    p.finishRespawn();
    assert(!p.dead());
    return 0;
}

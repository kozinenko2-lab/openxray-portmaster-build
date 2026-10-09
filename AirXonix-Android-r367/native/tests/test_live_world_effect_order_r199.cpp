#include <cassert>
#include <cmath>
#include "game/game.hpp"

struct GameTestProbe {
    static void setSlow(Game& g,float v){ g.enemySpeedFactor_=v; }
    static void death(Game& g){ g.levelIntroScene_.elapsedMs=g.levelIntroScene_.durationMs; g.levelEntryScene_.running=false; g.handleDeath(); }
    static void finale(Game& g){ g.levelIntroScene_.elapsedMs=g.levelIntroScene_.durationMs; g.levelEntryScene_.running=false; g.beginFinalSequence(); }
    static void finaleY(Game& g,float y){ g.finalScene_.presentationY=y; g.finalScene_.xonixHeight=std::min(.04f,y); }
};

static bool near(float a,float b,float e=1e-6f){return std::fabs(a-b)<=e;}

int main(){
    // 0x41C816: death-world executes 0x4185F0 every live frame. A slowed
    // enemy factor therefore recovers in the death phase rather than waiting
    // until gameplay resumes.
    {
        Game g;
        GameTestProbe::setSlow(g,.5f);
        GameTestProbe::death(g);
        g.update(InputState{},100);
        assert(near(g.enemySpeedFactor(),.51f));
    }

    // 0x41B4BE..0x41B5C2: finale calls 0x4185F0 while raw Y <= .1.
    {
        Game g;
        GameTestProbe::setSlow(g,.5f);
        GameTestProbe::finale(g);
        GameTestProbe::finaleY(g,.097f);
        g.update(InputState{},100); // raw Y=.099 -> recovery runs
        assert(near(g.enemySpeedFactor(),.51f));
    }

    // The first frame whose raw Y overshoots .1 clamps but skips 0x4185F0;
    // later frames remain in that branch, so the slow factor stays unchanged.
    {
        Game g;
        GameTestProbe::setSlow(g,.5f);
        GameTestProbe::finale(g);
        GameTestProbe::finaleY(g,.099f);
        g.update(InputState{},100); // raw Y=.101 -> clamp, no recovery
        assert(near(g.finalScene().presentationY,.10000000149f));
        assert(near(g.enemySpeedFactor(),.5f));
        g.update(InputState{},100);
        assert(near(g.enemySpeedFactor(),.5f));
    }
    return 0;
}

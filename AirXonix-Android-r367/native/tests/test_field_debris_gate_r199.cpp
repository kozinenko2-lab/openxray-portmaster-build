#include "game/game.hpp"
#include <cassert>
#include <cmath>

struct EntitiesTestProbe {
    static FieldDebrisParticle& first(Entities& e){return e.fieldDebris_[0];}
};
struct GameTestProbe {
    static void beginInterLevel(Game& g,std::size_t next){g.levelIntroScene_.elapsedMs=g.levelIntroScene_.durationMs; g.levelEntryScene_.running=false; g.beginInterLevel(next);}
    static void beginFinal(Game& g){g.levelIntroScene_.elapsedMs=g.levelIntroScene_.durationMs; g.levelEntryScene_.running=false; g.beginFinalSequence();}
    static Entities& entities(Game& g){return g.entities_;}
};

static bool near(float a,float b,float e=1e-8f){return std::fabs(a-b)<=e;}

int main(){
    InputState idle{};
    Game g;
    auto& q=EntitiesTestProbe::first(GameTestProbe::entities(g));
    q={.5f,.1f,.5f,0.f,0.0002f,0.f};

    GameTestProbe::beginInterLevel(g,g.levelIndex()+1);
    assert(g.displayFieldDebris());
    g.update(idle,1000); // remaining 3000, 0x417700 runs.
    const float vyAfterFirst=q.vy;
    assert(vyAfterFirst<0.0002f);

    g.update(idle,1001); // remaining 1999, 0x417700 is gated off.
    assert(!g.displayFieldDebris());
    assert(near(q.vy,vyAfterFirst));

    // Finale's 0x41BCDD gate compares boolean inputArmed (0/1) against 2000,
    // so 0x417700 is unreachable for the whole finale.
    Game f;
    auto& fq=EntitiesTestProbe::first(GameTestProbe::entities(f));
    fq={.5f,.1f,.5f,0.f,0.0002f,0.f};
    GameTestProbe::beginFinal(f);
    const float finalVy=fq.vy;
    assert(!f.displayFieldDebris());
    f.update(idle,16);
    assert(near(fq.vy,finalVy));
    assert(!f.displayFieldDebris());
    return 0;
}

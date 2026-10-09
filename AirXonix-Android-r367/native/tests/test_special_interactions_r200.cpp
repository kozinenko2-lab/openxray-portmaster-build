#include "game/game.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

struct SpecialObjectsTestProbe {
    static HomingSpecial& homing(SpecialObjects& s){ return s.homing_; }
    static int& homingSfx(SpecialObjects& s){ return s.homingSfx22Events_; }
    static bool redirected(const SpecialObjects& s){ return s.externalRedirect_; }
    static EraserSpecial& eraser(SpecialObjects& s){ return s.eraser_; }
};

struct EntitiesTestProbe {
    static std::vector<AirEnemy>& air(Entities& e){ return e.air_; }
};

struct GameTestProbe {
    static SpecialObjects& special(Game& g){ return g.specialObjects_; }
    static Entities& entities(Game& g){ return g.entities_; }
    static void resolveSpecialPreEntityContacts(Game& g){ g.resolveSpecialPreEntityContacts(); }
    static void consumeSpecialPostUpdateEvents(Game& g){ g.consumeSpecialPostUpdateEvents(); }
};

static bool near(float a,float b,float eps=1e-7f){ return std::fabs(a-b)<=eps; }

int main(){
    // 0x4154E6..0x415558: 0x257D9FC is baked into the cached homing vector.
    Field field;
    Player player; player.reset();
    LegacyRandom rng(1);
    SpecialObjects s;
    auto& h=SpecialObjectsTestProbe::homing(s);
    h.active=true;
    h.speed=.001f;
    h.worldX=player.worldX()-.01f;
    h.worldZ=player.worldZ();
    h.height=.25f;
    h.retargetClock=0;
    s.update(1,field,player,.3f,rng);
    assert(near(h.directionX,.3f,2e-6f));
    const float x1=h.worldX;
    // Enter the no-retarget window and change the live factor. Original keeps
    // the old .3 magnitude until retarget resumes; it does not jump to 1.0.
    h.retargetClock=0x1000;
    s.update(1,field,player,1.f,rng);
    assert(near(h.worldX-x1,.0003f,2e-7f));

    // Literal EXE quirk: this is a bitwise mask, not modulo 0x1400.
    // 0x0C00 + 1 becomes 0x0001 after &0x13FF, so retarget resumes.
    h.retargetClock=0x0c00;
    h.worldX=player.worldX()-.01f; h.worldZ=player.worldZ();
    h.directionX=123.0f; h.directionZ=123.0f;
    s.update(1,field,player,.30f,rng);
    assert(h.retargetClock==1);
    assert(std::fabs(h.directionX-0.30f) < 1e-5f);
    assert(near(h.directionX,.3f,2e-6f));

    // 0x415560..0x415583 has no post-step floor clamp.
    h.speed=.001f;
    h.directionX=h.directionZ=0.f;
    h.retargetClock=0x1000;
    h.height=.0225f;
    s.update(20,field,player,1.f,rng);
    assert(h.height<.022f);
    const float landed=h.height;
    s.update(20,field,player,1.f,rng);
    assert(near(h.height,landed));

    // 0x415621..0x415650: phase wrap generates positional logical SFX 0x22.
    h.soundPhase=6.25f;
    s.update(10,field,player,1.f,rng);
    assert(s.consumeHomingSfx22Events()==1);
    assert(s.consumeHomingSfx22Events()==0);

    // 0x41612E..0x41623A: eraser/air contact radius .01 and component-wise
    // reflection away from the eraser.
    Entities entities;
    auto& air=EntitiesTestProbe::air(entities);
    air.clear();
    AirEnemy e;
    e.active=true;
    e.worldX=.505f; e.worldZ=.505f;
    e.vx=-.02f; e.vy=-.03f;
    air.push_back(e);
    assert(entities.resolveEraserAirContacts(.5f,.5f));
    assert(near(air[0].vx,.02f));
    assert(near(air[0].vy,.03f));
    air[0].worldX=.52f; air[0].worldZ=.52f;
    assert(!entities.resolveEraserAirContacts(.5f,.5f));

    // Game wiring: the contact raises the one-shot eraser redirect flag, and
    // the homing pulse is routed through the spatial audio event queue.
    Game g;
    auto& ge=GameTestProbe::entities(g);
    auto& ga=EntitiesTestProbe::air(ge);
    ga.clear();
    AirEnemy ge0; ge0.active=true; ge0.worldX=.505f; ge0.worldZ=.505f; ge0.vx=-.01f; ge0.vy=-.01f;
    ga.push_back(ge0);
    auto& gs=GameTestProbe::special(g);
    auto& er=SpecialObjectsTestProbe::eraser(gs);
    er.active=true; er.speed=.000002f; er.worldX=.5f; er.worldZ=.5f;
    GameTestProbe::resolveSpecialPreEntityContacts(g);
    assert(SpecialObjectsTestProbe::redirected(gs));

    auto& gh=SpecialObjectsTestProbe::homing(gs);
    gh.active=true; gh.worldX=.51f; gh.height=.02f; gh.worldZ=.49f;
    SpecialObjectsTestProbe::homingSfx(gs)=1;
    (void)g.takeDeathAudioEvents();
    GameTestProbe::consumeSpecialPostUpdateEvents(g);
    const auto ev=g.takeDeathAudioEvents();
    assert(ev.size()==1);
    assert(ev[0].kind==DeathAudioEventKind::SpatialPlay);
    assert(ev[0].logicalId==0x22u);
    assert(near(ev[0].x,.51f) && near(ev[0].y,.02f) && near(ev[0].z,.49f));
    assert(near(ev[0].scalar,.1f));

    std::cout << "special interactions r200 ok\n";
    return 0;
}

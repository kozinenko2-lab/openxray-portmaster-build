#include "game/game.hpp"
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

struct EntitiesTestProbe {
    static void setAir(Entities& e,std::vector<AirEnemy> v){ e.air_=std::move(v); }
    static void setGround(Entities& e,std::vector<GroundEnemy> v){ e.ground_=std::move(v); }
    static void pushAudio(Entities& e,const EntitySpatialSfxEvent& ev){ e.collisionSfxEvents_.push_back(ev); }
};

struct GameTestProbe {
    static Entities& entities(Game& g){ return g.entities_; }
    static void consumeEntityCollisionAudioEvents(Game& g){ g.consumeEntityCollisionAudioEvents(); }
};

static LevelRecord emptyLevel(){ return LevelRecord{}; }
static bool near(float a,float b,float eps=1e-7f){ return std::fabs(a-b)<=eps; }
static bool check(bool c,const std::string& what){
    if(c) return true;
    std::cerr << "FAIL: " << what << "\n";
    return false;
}

int main(){
    bool ok=true;

    // 0x416486..0x4164AC: air-air contact => positional SFX 3 / bol2,
    // first airborne record cached X/Z, Y=.004, scalar 1.0.
    {
        Entities e; Field f; f.build(emptyLevel()); LegacyRandom rng(1u);
        AirEnemy a{},b{};
        a.x=10.f;a.y=10.f;a.worldX=.431f;a.worldZ=.432f;a.vx=.01f;
        b.x=11.f;b.y=10.f;b.worldX=.441f;b.worldZ=.442f;b.vx=-.01f;
        EntitiesTestProbe::setAir(e,{a,b});
        e.update(1,f,rng,1.f,false,false);
        const auto ev=e.consumeCollisionSfxEvents();
        ok &= check(ev.size()==1,"air-air overlap must emit exactly one collision SFX in this setup");
        if(ev.size()==1){
            ok &= check(ev[0].logicalId==0x03u,"air-air contact must use logical SFX 3 / bol2");
            ok &= check(near(ev[0].x,.431f)&&near(ev[0].y,.004f)&&near(ev[0].z,.432f),"air-air SFX position must use first record cached X/.004/Z");
            ok &= check(near(ev[0].scalar,1.f),"air-air SFX scalar must be 1.0");
        }
    }

    // 0x4165C1..0x4165E7: airborne occupied-field contact => SFX 2 / bol1.
    {
        Entities e; Field f; f.build(emptyLevel()); LegacyRandom rng(1u);
        AirEnemy a{};a.x=10.f;a.y=10.f;a.vx=1.f;a.worldX=.451f;a.worldZ=.452f;
        EntitiesTestProbe::setAir(e,{a});
        f.set(10,11,Field::Safe); // neighbour of rounded candidate (11,10)
        e.update(1,f,rng,1.f,false,false);
        const auto ev=e.consumeCollisionSfxEvents();
        ok &= check(ev.size()==1,"one airborne field contact must emit one bol1 event");
        if(ev.size()==1){
            ok &= check(ev[0].logicalId==0x02u,"air-field contact must use logical SFX 2 / bol1");
            ok &= check(near(ev[0].x,.451f)&&near(ev[0].y,.004f)&&near(ev[0].z,.452f),"air-field SFX must use cached airborne X/.004/Z");
            ok &= check(near(ev[0].scalar,1.f),"air-field SFX scalar must be 1.0");
        }
    }

    // 0x416C38..0x416C80: crawler pair SFX 1 / min0 has a shared strict
    // >50-ms debounce. Every collision refreshes the stamp, even if muted.
    {
        Entities e; Field f; f.build(emptyLevel()); LegacyRandom rng(1u);
        GroundEnemy a{},b{};
        a.x=10.f;a.y=10.f;a.worldX=.461f;a.worldZ=.462f;a.respawnDelay=0.f;
        b.x=11.f;b.y=10.f;b.worldX=.471f;b.worldZ=.472f;b.respawnDelay=0.f;
        f.set(10,10,Field::Safe);f.set(11,10,Field::Safe);
        EntitiesTestProbe::setGround(e,{a,b});
        e.update(1,f,rng,1.f,false,false);
        auto ev=e.consumeCollisionSfxEvents();
        ok &= check(ev.size()==1 && ev[0].logicalId==0x01u,"first crawler pair contact must play logical SFX 1");
        if(!ev.empty()){
            ok &= check(near(ev[0].x,.461f)&&near(ev[0].y,.009f)&&near(ev[0].z,.462f),"crawler pair SFX must use outer record cached X/.009/Z");
            ok &= check(near(ev[0].scalar,1.f),"crawler pair SFX scalar must be 1.0");
        }
        e.update(50,f,rng,1.f,false,false); // delta exactly 50: strictly suppressed
        ev=e.consumeCollisionSfxEvents();
        ok &= check(ev.empty(),"crawler collision at exactly 50 ms must be suppressed");
        e.update(51,f,rng,1.f,false,false); // prior suppressed contact refreshed stamp, now delta 51
        ev=e.consumeCollisionSfxEvents();
        ok &= check(ev.size()==1 && ev[0].logicalId==0x01u,"crawler collision after a 51-ms gap must play again");
    }

    // 0x416DFE..0x416E46: crawler field contact uses the same debounce and
    // logical SFX 0 / min0, cached X/Z, Y=.009, scalar .5.
    {
        Entities e; Field f; f.build(emptyLevel()); LegacyRandom rng(1u);
        GroundEnemy a{};a.x=10.f;a.y=10.f;a.vx=1.f;a.worldX=.481f;a.worldZ=.482f;a.respawnDelay=0.f;
        EntitiesTestProbe::setGround(e,{a});
        // Empty interior is blocked to crawler, so the first movement quantum collides.
        e.update(1,f,rng,1.f,false,false);
        const auto ev=e.consumeCollisionSfxEvents();
        ok &= check(ev.size()==1 && ev[0].logicalId==0x00u,"crawler field contact must use logical SFX 0");
        if(!ev.empty()){
            ok &= check(near(ev[0].x,.481f)&&near(ev[0].y,.009f)&&near(ev[0].z,.482f),"crawler field SFX must use cached X/.009/Z");
            ok &= check(near(ev[0].scalar,.5f),"crawler field SFX scalar must be .5");
        }
    }

    // Game wiring: entity collision events are drained into the existing
    // spatial-audio event queue rather than remaining renderer-only counters.
    {
        Game g;
        (void)g.takeDeathAudioEvents();
        EntitiesTestProbe::pushAudio(GameTestProbe::entities(g),{0x03u,.51f,.004f,.49f,1.f});
        GameTestProbe::consumeEntityCollisionAudioEvents(g);
        const auto ev=g.takeDeathAudioEvents();
        ok &= check(ev.size()==1,"Game must drain entity collision audio events");
        if(ev.size()==1){
            ok &= check(ev[0].kind==DeathAudioEventKind::SpatialPlay && ev[0].logicalId==0x03u,"drained collision event must remain positional SFX 3");
            ok &= check(near(ev[0].x,.51f)&&near(ev[0].y,.004f)&&near(ev[0].z,.49f)&&near(ev[0].scalar,1.f),"Game must preserve collision SFX spatial arguments");
        }
    }

    if(!ok) return 1;
    std::cout << "collision audio r203 ok\n";
    return 0;
}

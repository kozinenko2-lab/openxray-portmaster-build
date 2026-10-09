#include "game/game.hpp"
#include "game/pickups.hpp"
#include <cassert>
#include <cmath>

struct PickupsTestProbe {
    static void parkAll(Pickups& p){
        for(std::size_t i=0;i<Pickups::Count;++i){
            p.items_[i].timerMs=1000000;
            p.items_[i].gridX=-100-int(i);
            p.items_[i].gridY=-100;
            p.items_[i].worldX=0.2f+0.02f*float(i);
            p.items_[i].worldZ=0.2f;
            p.items_[i].height=0.105f;
            p.audioLoopActive_[i]=false;
        }
        p.audioEvents_.clear();
        p.smashEvents_.clear();
    }
    static Pickup& item(Pickups& p,std::size_t i){return p.items_[i];}
};

struct GameTestProbe {
    static void beginInterLevel(Game& g,std::size_t next){g.levelIntroScene_.elapsedMs=g.levelIntroScene_.durationMs; g.levelEntryScene_.running=false; g.capturePercent_=100; g.beginInterLevel(next);}
    static Pickups& pickups(Game& g){return g.pickups_;}
};

static bool near(float a,float b,float e=1e-6f){return std::fabs(a-b)<=e;}

int main(){
    // 0x417B71..0x417C08: pickup Y < .012 and Xonix Y < .03 are independent,
    // strict predicates. This is the cinematic collection gate.
    {
        Pickups p; Field f; Entities e; LegacyRandom rng(1u);
        PickupsTestProbe::parkAll(p);
        auto& q=PickupsTestProbe::item(p,0);
        q.timerMs=0; q.worldX=.5f; q.worldZ=.5f; q.height=.008f;
        auto hit=p.updateWithCollector(1,f,{.5f,.029f,.5f},e,rng,60000,3);
        assert(hit.size()==1 && hit[0]==PickupEffect::Score1000);

        PickupsTestProbe::parkAll(p);
        auto& q2=PickupsTestProbe::item(p,0);
        q2.timerMs=0; q2.worldX=.5f; q2.worldZ=.5f; q2.height=.008f;
        auto blocked=p.updateWithCollector(1,f,{.5f,.030f,.5f},e,rng,60000,3);
        assert(blocked.empty());
        assert(q2.timerMs==0); // no respawn => no collection at exactly .03
    }

    // 0x41B560 pre-pass: below-.01 pickup always respawns, but 0x418040 is
    // emitted only while the finale scene-light scalar is strictly >100.
    {
        Pickups p; Field f; LegacyRandom rng(7u);
        PickupsTestProbe::parkAll(p);
        auto& q=PickupsTestProbe::item(p,0);
        q.timerMs=2000; q.height=.009f; q.worldX=.45f; q.worldZ=.45f;
        assert(p.prepareFinalSceneFrame(f,rng,60000,255.f)==1);
        auto smash=p.consumeSmashEvents();
        assert(smash.size()==1);
        assert(near(q.height,.105f,1e-5f));

        PickupsTestProbe::parkAll(p);
        auto& q2=PickupsTestProbe::item(p,0);
        q2.timerMs=2000; q2.height=.009f; q2.worldX=.45f; q2.worldZ=.45f;
        assert(p.prepareFinalSceneFrame(f,rng,60000,100.f)==0);
        assert(p.consumeSmashEvents().empty());
        assert(near(q2.height,.105f,1e-5f));
    }

    // Runtime integration: early inter-level pickup is actually awarded and
    // its collection SFX leaves the pickup subsystem through the normal queue.
    {
        Game g;
        auto& p=GameTestProbe::pickups(g);
        PickupsTestProbe::parkAll(p);
        auto& q=PickupsTestProbe::item(p,0);
        q.timerMs=0;
        q.worldX=g.player().worldX(); q.worldZ=g.player().worldZ(); q.height=.008f;
        GameTestProbe::beginInterLevel(g,g.levelIndex()+1);
        const int before=g.score(); // r272 completion prologue has already run.
        g.takeDeathAudioEvents();
        InputState idle{};
        g.update(idle,16);
        assert(g.phase()==GamePhase::InterLevel);
        assert(g.score()==before+1000);
        bool collectionCue=false;
        for(const auto& ev:g.takeDeathAudioEvents())
            if(ev.kind==DeathAudioEventKind::SpatialPlay && ev.logicalId==0x0Au) collectionCue=true;
        assert(collectionCue);
    }
    return 0;
}

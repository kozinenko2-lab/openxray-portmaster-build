#include "pickups.hpp"
#include "entities.hpp"
#include "player.hpp"
#include "legacy_particles.hpp"
#include <algorithm>
#include <cmath>

namespace {
constexpr float kCellStep=0.003125f;
constexpr float kPickupBase=0.4015384614467621f; // 0x43B494
constexpr float kInitialHeight=0.1049999967f;
constexpr float kPlayerCollisionSq=4.9e-5f;
constexpr float kEnemyCollisionSq=2.5e-5f;
PickupSmashEvent makePickupSmashEvent(const Pickup& p,std::size_t type,LegacyRandom& rng){
    PickupSmashEvent ev; ev.worldX=p.worldX; ev.height=p.height; ev.worldZ=p.worldZ; ev.type=int(type);
    for(auto& q:ev.particles){
        const auto v=LegacyParticles::pickupVelocity(rng);
        q.vx=v.x;q.vy=v.y;q.vz=v.z;
    }
    return ev;
}

}

void Pickups::respawn(std::size_t index,const Field&,LegacyRandom& rng,int levelTimerMs){
    Pickup& p=items_[index];
    // 0x4177C6..0x4177E2: each pickup owns one retained spatial voice handle.
    // Respawn always stops an active falling/flight loop before choosing a new position.
    if(audioLoopActive_[index]){
        audioEvents_.push_back({PickupAudioEventKind::SpatialStop,index,0u,p.worldX,p.height,p.worldZ,1.f});
        audioLoopActive_[index]=false;
    }
    for(;;){
        const int x=rng.mask(63),y=rng.mask(63);
        bool reject=false;
        for(std::size_t i=0;i<Count;++i){
            if(i==index)continue;
            const Pickup& o=items_[i];
            const float dx=float(o.gridX-x),dy=float(o.gridY-y);
            if(dx*dx+dy*dy<25.f){reject=true;break;}
        }
        if(reject)continue;
        p.gridX=x;p.gridY=y;break;
    }
    p.worldX=float(p.gridX)*kCellStep+kPickupBase;
    p.worldZ=float(p.gridY)*kCellStep+kPickupBase;
    p.height=kInitialHeight;

    switch(index){
        case 0:p.timerMs=16000+1200*int(rng.mask(7));break;
        case 1:{
            p.timerMs=12000+1100*int(rng.mask(7));
            if(p.timerMs>levelTimerMs)p.timerMs=levelTimerMs/2;
            break;
        }
        case 2:p.timerMs=1000000;break;
        case 3:p.timerMs=10000+800*int(rng.mask(7));break;
        case 4:p.timerMs=1000*(int(rng.mask(7))+10);break;
        default:p.timerMs=9000+1200*int(rng.mask(7));break;
    }
}

void Pickups::reset(const Field& field,LegacyRandom& rng,int levelTimerMs){
    // 0x419133 resets legacy global 0x0257DA9C during gameplay setup.
    presentationAngle_=0;
    // r362 DIRECT EXE 0x419174..0x419196 -> 0x4177C0: the six 24-byte
    // pickup records are NOT cleared before sequential respawn. During slot 0
    // placement, slots 1..5 still contain their previous-level coordinates
    // (or BSS zeroes on the first level); each later slot sees the already-new
    // earlier slots plus the still-old later slots. Do not replace them with
    // artificial sentinels: doing so changes rejection and the global rand()
    // stream for the entire level.
    for(std::size_t i=0;i<Count;++i){
        respawn(i,field,rng,levelTimerMs);
        // 0x419174..0x419196: initial level setup calls 0x4177C0 for each
        // slot and then overwrites the slot timer with exactly 0x1388 (5000).
        // Later respawns keep their per-type randomized delays.
        items_[i].timerMs=5000;
    }
}

float Pickups::targetHeight(const Pickup& p,const Field& field) const{
    if(p.gridX<2||p.gridX>62||p.gridY<2||p.gridY>62)return .008f;
    static constexpr int dx[8]={1,-1,0,0,1,-1,1,-1};
    static constexpr int dy[8]={0,0,1,-1,1,-1,-1,1};
    std::uint8_t bits=0;
    for(int i=0;i<8;++i)bits|=field.at(p.gridX+dx[i],p.gridY+dy[i]);
    if(bits==0)return 0.f;
    if(bits>=Field::Safe)return .008f;
    // Temporary capture markers are 1..15. The field stores the exact legacy
    // 0..0.008 animation phase, which doubles as the pickup's local surface Y.
    if(bits<16 && field.markerActive(bits))return std::min(.008f,field.markerPhase(bits));
    return 0.f;
}

PickupEffect Pickups::resolveRandomEffect(LegacyRandom& rng,int lives){
    int choice=int((rng.next()>>2)&7u);
    // 0x4156C0 remaps random slot 2 to the separate effect #8.
    if(choice==2)return PickupEffect::CameraShake;
    if(choice==3){
        // A random extra life is only possible below five lives and only on a
        // second random roll. Otherwise this slot becomes the enemy slowdown.
        if(int(rng.mask(31))<10 && lives<5)return PickupEffect::ExtraLife;
        return PickupEffect::SlowEnemies;
    }
    return static_cast<PickupEffect>(choice);
}

std::vector<PickupEffect> Pickups::update(int dtMs,const Field& field,const Player& player,
                                          const Entities& entities,LegacyRandom& rng,int levelTimerMs,int lives,
                                          bool allowPlayerCollection){
    return updateWithCollector(dtMs,field,
        {player.worldX(),player.visualY(),player.worldZ()},entities,rng,levelTimerMs,lives,
        allowPlayerCollection);
}

std::vector<PickupEffect> Pickups::updateWithCollector(int dtMs,const Field& field,
                                          const PickupCollectorState& collector,
                                          const Entities& entities,LegacyRandom& rng,int levelTimerMs,int lives,
                                          bool allowPlayerCollection){
    std::vector<PickupEffect> result;
    if(dtMs<=0)return result;
    for(std::size_t i=0;i<Count;++i){
        Pickup& p=items_[i];
        if(p.timerMs>0){
            // The original tests >0 before subtracting and does not activate a
            // pickup until the following update if the counter crosses zero.
            p.timerMs-=dtMs;
            continue;
        }

        const float target=targetHeight(p,field);
        // 0x417A86..0x417B6E: falling pickup audio and exact one-frame landing semantics.
        // The original subtracts dt*1e-4 without clamping; only the following update
        // snaps an overshoot back to target, stops retained bfly (ID 0x0F), and plays
        // one-shot bpop (ID 0x10) at the landed position.
        if(target<p.height){
            p.height-=float(dtMs)*0.00009999999747378752f;
            if(!audioLoopActive_[i]){
                audioLoopActive_[i]=true;
                audioEvents_.push_back({PickupAudioEventKind::SpatialStart,i,0x0Fu,p.worldX,p.height,p.worldZ,1.f});
            }
            audioEvents_.push_back({PickupAudioEventKind::SpatialUpdate,i,0u,p.worldX,p.height,p.worldZ,1.f});
        }else{
            if(target>p.height)p.height=target;
            if(audioLoopActive_[i]){
                audioEvents_.push_back({PickupAudioEventKind::SpatialStop,i,0u,p.worldX,p.height,p.worldZ,1.f});
                audioLoopActive_[i]=false;
                audioEvents_.push_back({PickupAudioEventKind::SpatialPlay,i,0x10u,p.worldX,p.height,p.worldZ,1.f});
            }
        }

        const float pdx=p.worldX-collector.worldX,pdz=p.worldZ-collector.worldZ;
        // DIRECT EXE 0x417B71..0x417C08: BOTH vertical predicates matter.
        // Pickup Y must be < .012, and Xonix/presentation Y must be strictly
        // < .03. The latter is what naturally turns collection off after the
        // first ~1 second of inter-level/finale rise without any cinematic flag.
        if(allowPlayerCollection && p.height<0.012000000104308128f &&
           collector.worldY<0.029999999329447746f &&
           sq(pdx)+sq(pdz)<kPlayerCollisionSq){
            // Pickup slot #5 enters dispatcher code 6: resolve its random effect
            // before respawning so the global MSVC rand() stream stays in the
            // same order as the original.
            const PickupEffect effect=i==5?resolveRandomEffect(rng,lives):static_cast<PickupEffect>(int(i));
            result.push_back(effect);
            // DIRECT EXE 0x415804..0x41586C: collection has an immediate spatial
            // cue in addition to the later auxiliary announcer voice. Effects
            // 0..3 use bon0..bon3 (ids 0x0A..0x0D), effect 4 uses whip (0x31),
            // and surprise effects 5..8 use sur1 (0x30).
            const int effectId=static_cast<int>(effect);
            const std::size_t collectionSfx=(effectId<=3)?std::size_t(0x0A+effectId)
                :(effectId==4?std::size_t(0x31):std::size_t(0x30));
            audioEvents_.push_back({PickupAudioEventKind::SpatialPlay,i,collectionSfx,p.worldX,p.height,p.worldZ,1.f});
            respawn(i,field,rng,levelTimerMs);
            continue;
        }

        bool destroyed=false;
        if(p.height<.01f){
            for(const auto& e:entities.ground()){
                if(!e.active||e.respawnDelay>0.f)continue;
                const float dx=p.worldX-e.worldX,dz=p.worldZ-e.worldZ;
                if(dx*dx+dz*dz<kEnemyCollisionSq){destroyed=true;break;}
            }
        }
        if(!destroyed&&p.height<.004f){
            for(const auto& e:entities.air()){
                if(!e.active)continue;
                const float dx=p.worldX-e.worldX,dz=p.worldZ-e.worldZ;
                if(dx*dx+dz*dz<kEnemyCollisionSq){destroyed=true;break;}
            }
        }
        if(destroyed){
            // 0x418040 pickup-smash path: enemy impact is not a silent despawn.
            smashEvents_.push_back(makePickupSmashEvent(p,i,rng));
            respawn(i,field,rng,levelTimerMs);
        }
    }
    // 0x417D4C..0x417D5C: angle += 2*dt after all six pickup records.
    presentationAngle_ += dtMs*2;
    return result;
}

int Pickups::prepareFinalSceneFrame(const Field& field,LegacyRandom& rng,int levelTimerMs,float sceneLight255){
    int smashed=0;
    for(std::size_t i=0;i<Count;++i){
        Pickup& p=items_[i];
        // 0x41B565..0x41B57C: long timers are forced into 400..904 ms.
        if(p.timerMs>1000)p.timerMs=400+8*int(rng.mask(63));

        // 0x41B57E..0x41B5AE happens BEFORE the ordinary 0x4179B0 update.
        // A pickup already below .01 is always respawned. 0x418040 (128
        // smash particles + SFX 0x0E) is emitted only while light > 100.
        if(p.height<0.009999999776482582f){
            if(sceneLight255>100.0f){
                ++smashed;
                smashEvents_.push_back(makePickupSmashEvent(p,i,rng));
            }
            respawn(i,field,rng,levelTimerMs);
        }
    }
    return smashed;
}


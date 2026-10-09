#pragma once
#include <array>
#include <cstddef>
#include <vector>
#include <utility>
#include "core/legacy_random.hpp"
#include "field.hpp"

class Player;
class Entities;

struct PickupsTestProbe;

struct PickupCollectorState {
    float worldX=0.f;
    float worldY=0.00800000037997961f;
    float worldZ=0.f;
};

// Reconstructed from 0x254A1F0: six records, 24 bytes each in the x86 build.
struct PickupSmashParticle { float vx=0.f,vy=0.f,vz=0.f; };
struct PickupSmashEvent {
    float worldX=0.f,height=0.f,worldZ=0.f;
    int type=0;
    std::array<PickupSmashParticle,128> particles{};
};

enum class PickupAudioEventKind { SpatialStart, SpatialUpdate, SpatialStop, SpatialPlay };
struct PickupAudioEvent {
    PickupAudioEventKind kind=PickupAudioEventKind::SpatialPlay;
    std::size_t slot=0;
    std::size_t logicalId=0;
    float x=0.f,y=0.f,z=0.f,scalar=1.f;
};

struct Pickup {
    int timerMs=0;      // +0x00: positive = hidden/respawn delay
    int gridX=0;        // +0x04
    int gridY=0;        // +0x08
    float worldX=0.f;   // +0x0c
    float height=.105f; // +0x10
    float worldZ=0.f;   // +0x14
};

// Effect numbers intentionally mirror the original 0x4156C0 dispatcher.
// Effect ids mirror 0x4156C0. Dispatcher input 6 is also used by pickup #5
// as the random-effect wrapper before it resolves to one of ids 0..8.
enum class PickupEffect : int {
    Score1000=0,
    Time15000=1,
    ExtraLife=2,
    SlowEnemies=3,
    PlayerFast=4,
    PlayerSlow=5,
    CameraZoomIn=6,
    Blackout=7,
    CameraShake=8
};

class Pickups {
public:
    static constexpr std::size_t Count=6;
    void reset(const Field& field,LegacyRandom& rng,int levelTimerMs);
    // Returns effects granted to the player during this update. Enemy/special
    // destruction simply respawns the pickup and produces no event.
    std::vector<PickupEffect> update(int dtMs,const Field& field,const Player& player,
                                     const Entities& entities,LegacyRandom& rng,int levelTimerMs,int lives,
                                     bool allowPlayerCollection=true);
    // Same literal 0x4179B0 path, but with the current presentation X/Y/Z.
    // Cinematics update the legacy global Xonix coordinates independently of
    // Player's logical grid record, so inter-level/death/finale must use this.
    std::vector<PickupEffect> updateWithCollector(int dtMs,const Field& field,
                                     const PickupCollectorState& collector,
                                     const Entities& entities,LegacyRandom& rng,int levelTimerMs,int lives,
                                     bool allowPlayerCollection=true);
    const std::array<Pickup,Count>& items() const{return items_;}
    // 0x0257DA9C: shared pickup presentation angle. 0x417D55 adds 2*dt
    // once per pickup update; legacy rotation helpers mask to 0..2047.
    int presentationAngle2048() const{return presentationAngle_ & 0x7ff;}

    // 0x41957E..0x419597: after enough newly captured cells accumulate,
    // pickup slot #2 (the extra-life pickup) is forced to appear soon. The
    // original clamps only delays greater than 1000 ms; already-short delays
    // are preserved exactly.
    void forceExtraLifePickupSoon(){
        if(items_[2].timerMs>1000)items_[2].timerMs=1000;
    }

    // Exact random wrapper used by pickup #5. It preserves the original RNG
    // consumption and the <=4-life guard around a randomly awarded life.
    static PickupEffect resolveRandomEffect(LegacyRandom& rng,int lives);

    // 0x41B560..0x41B5B5: final-mode cinematic pickup loop. Hidden
    // pickups are forced to 400..904 ms respawn delays; once a falling pickup
    // reaches the field it smashes and immediately respawns. Returns the
    // number of smash events produced this frame for future particles/audio.
    // 0x41B560..0x41B5B5 is a PRE-PASS before the normal 0x4179B0 call:
    // shorten long timers; if a pickup is already below .01, optionally emit
    // 0x418040 while sceneLight255>100, then always respawn it.
    int prepareFinalSceneFrame(const Field& field,LegacyRandom& rng,int levelTimerMs,float sceneLight255);
    std::vector<PickupSmashEvent> consumeSmashEvents(){ auto out=std::move(smashEvents_);smashEvents_.clear();return out; }
    std::vector<PickupAudioEvent> consumeAudioEvents(){ auto out=std::move(audioEvents_);audioEvents_.clear();return out; }
private:
    friend struct PickupsTestProbe;
    void respawn(std::size_t index,const Field& field,LegacyRandom& rng,int levelTimerMs);
    float targetHeight(const Pickup& p,const Field& field) const;
    static float sq(float v){return v*v;}
    std::array<Pickup,Count> items_{};
    int presentationAngle_=0;
    std::vector<PickupSmashEvent> smashEvents_;
    std::array<bool,Count> audioLoopActive_{};
    std::vector<PickupAudioEvent> audioEvents_;
};

#pragma once
#include <cstdint>
#include <vector>
#include <array>
#include "core/legacy_random.hpp"
#include "field.hpp"
#include "level.hpp"

// Semantic reconstruction of the 80-byte original flying-enemy record.
// The native renderer does not need to store the original 40-byte transform
// blob, but the gameplay-facing fields and angular state are kept explicitly.
struct AirEnemy {
    float x=32.f,y=32.f,vx=0.f,vy=0.f;
    int subtype=0;                 // original +0x10: 0..3
    int heading2048=0;             // original +0x14, 2048 units per revolution
    int angularAccumulator=0;      // original +0x18
    int spinStep=1;                // original +0x1c
    // Original +0x20/+0x24 are cached world coordinates. 0x415BA0 does not
    // initialize them: the static native record array therefore starts at BSS
    // zero and reused slots retain the previous level's cached values until the
    // first 0x416110 update rewrites them.
    float worldX=0.f,worldZ=0.f;
    bool active=true;
};

// Exact gameplay portion of the 28-byte crawler record.
struct FieldDebrisParticle {
    // Native pool is static/BSS: fresh records begin as all-zero. 0x4190B4
    // later writes Y=-1 only to the first 64 records on each level init.
    float x=0.f,y=0.f,z=0.f,vx=0.f,vy=0.f,vz=0.f;
    // 0x41766A compares the positive-float bit pattern against 0.005 and treats
    // <=0.005 as an emitter-free slot. This predicate is NOT the physics/render
    // gate: 0x417700 still updates/submits all 96 records.
    bool active() const { return y>0.004999999888241291f; }
};

struct EntitySpatialSfxEvent {
    std::size_t logicalId=0;
    float x=0.f,y=0.f,z=0.f,scalar=1.f;
};

struct GroundEnemy {
    float x=32.f,y=65.f,vx=0.f,vy=0.f;
    float respawnDelay=.5f;        // original +0x10: spawn/reset delay
    float worldX=1.f,worldZ=0.f;
    bool active=true;

    // The original retains stale cached +0x14/+0x18 world coordinates during
    // the vertical spawn phase. In the native renderer this makes the crawler
    // jump from a distant/previous position as it touches the board. Gameplay,
    // collisions and original cached fields remain unchanged; only the visual
    // presentation of a spawning crawler is anchored to its current grid cell.
    float presentationWorldX() const { return respawnDelay>0.f ? 0.4f+x*0.003125f : worldX; }
    float presentationWorldZ() const { return respawnDelay>0.f ? 0.4f+y*0.003125f : worldZ; }
};

class Entities {
public:
    void reset(const LevelRecord& level,const Field& field,LegacyRandom& rng);
    void update(int dtMs, Field& field,LegacyRandom& rng,float enemySpeedFactor=1.0f,
                bool deathRaiseCrawlers=false,bool updateDebris=true);
    // r282 / startup presentation: original 0x41D4D0 advances only 0x416110
    // (airborne enemies) during the 3-second level intro; crawlers are not stepped.
    void updateAirOnly(int dtMs, Field& field,LegacyRandom& rng,float enemySpeedFactor=1.0f);
    std::vector<GridSeed> captureSeeds() const;
    const std::vector<AirEnemy>& air() const { return air_; }
    const std::vector<GroundEnemy>& ground() const { return ground_; }
    const std::array<FieldDebrisParticle,96>& fieldDebris() const { return fieldDebris_; }
    // 0x41612E..0x41623A: when an active field eraser comes within .01 world
    // units of an airborne enemy, force each velocity component to point away
    // from the eraser while preserving its magnitude. Returns true if the
    // eraser's one-shot redirect flag must be raised.
    bool resolveEraserAirContacts(float eraserWorldX,float eraserWorldZ);
    bool consumeCrawlerPlayerHit(float worldX,float worldZ,float& hitX,float& hitZ);
    // 0x416890: transition/death respawn reseed for every crawler.
    void resetCrawlerTransition();
    bool consumeTrailHit(){ const bool v=!trailImpacts_.empty(); trailImpacts_.clear(); return v; }
    std::vector<GridSeed> consumeTrailImpacts(){ auto out=trailImpacts_; trailImpacts_.clear(); return out; }
    int consumeAirErosionImpactEvents(){ const int v=airErosionImpactEvents_; airErosionImpactEvents_=0; return v; }
    int consumeAirErosionDebrisHelperCalls(){ const int v=airErosionDebrisHelperCalls_; airErosionDebrisHelperCalls_=0; return v; }
    std::vector<EntitySpatialSfxEvent> consumeCollisionSfxEvents(){ auto out=collisionSfxEvents_; collisionSfxEvents_.clear(); return out; }
    friend struct EntitiesTestProbe;
    friend struct EntitiesTestProbeR100;
    friend struct EntitiesTestProbeR363;
    friend struct EntitiesTestProbeR364;
    friend struct EntitiesTestProbeR365;
private:
    static int gridRound(float v);
    static int legacyAirHeading2048(float vx,float vy);
    static bool airNearOccupied(const Field& f,int x,int y,bool* touchesTrail=nullptr);
    static void exchangeApproachingComponents(float ax,float ay,float bx,float by,
                                               float& avx,float& avy,float& bvx,float& bvy);
    void erodeAirImpact(Field& f,int x,int y,LegacyRandom& rng);
    void emitFieldDebris(int gridX,int gridY,LegacyRandom& rng);
    void updateFieldDebris(int dtMs);
    bool groundBlocked(const Field& f,int x,int y) const;
    void resetGroundEnemy(std::size_t index,GroundEnemy& e);
    std::vector<AirEnemy> air_;
    std::vector<GroundEnemy> ground_;
    std::array<FieldDebrisParticle,96> fieldDebris_{};
    bool trailHit_=false; // legacy compatibility summary; coordinate path uses trailImpacts_.
    std::vector<GridSeed> trailImpacts_;
    int airErosionImpactEvents_=0;
    int airErosionDebrisHelperCalls_=0;
    std::vector<EntitySpatialSfxEvent> collisionSfxEvents_;
    std::uint64_t crawlerCollisionClockMs_=0;
    std::uint64_t crawlerLastCollisionMs_=0;
    bool crawlerCollisionStampValid_=false;
};

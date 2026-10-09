#pragma once
#include "core/legacy_random.hpp"
#include "field.hpp"
#include "level.hpp"
#include <array>

class Player;

// Reconstructed from globals 0x254A2B0..0x254A2D4 / routines 0x415440 and
// 0x415490. It is enabled by LevelRecord::specialHoming and continuously
// steers toward the Xonix position.
struct HomingSpecial {
    bool active=false;
    float worldX=.5f;
    float height=.25f;
    float worldZ=.5f;
    float speed=0.f;
    // 0x4154E6..0x415530 stores (target-current) * enemySpeedFactor / distance,
    // not a unit vector.  The factor therefore remains cached during the
    // EXE masks with &0x13FF; reachable hold values are 0x1000..0x13FF.
    // (0x0400..0x0FFF cannot survive that non-contiguous bit mask.)
    float directionX=0.f;
    float directionZ=0.f;
    float spinPhase=0.f;
    float soundPhase=0.f;
    int retargetClock=0;
};

// Reconstructed from globals 0x254A288..0x254A2AC / routines 0x414DD0 and
// 0x414F30. This object travels on cardinal axes and removes a 4x4 block of
// field cells on impact. Air-enemy impacts use a different 8-neighbour eroder.
struct EraserDebrisParticle {
    // Native 0x257E8D8 pool is six floats (24 bytes) per record. The EXE
    // zeroes every record at 0x414F08 and never carries an active flag; all
    // 128 records are advanced/submitted every frame. `active` is retained
    // only for diagnostics/tests and must not gate runtime physics/rendering.
    float x=0.f,y=0.f,z=0.f,vx=0.f,vy=0.f,vz=0.f;
    bool active=false;
};

struct EraserSpecial {
    bool active=false;
    float worldX=.5f;
    float worldZ=.5f;
    float speed=0.f;
    float velocityX=0.f;
    float velocityZ=0.f;
    float travelBudget=0.f;
    float spinPhase=0.f;       // 0x0254A2A4: radians, drives bob/scale
    int rotationAngle2048=0;    // 0x0254A2A0: separate legacy Y rotation
};

class SpecialObjects {
public:
    friend struct SpecialObjectsTestProbe;
    friend struct SpecialObjectsTestProbe2;
    void reset(const LevelRecord& level,LegacyRandom& rng);
    void update(int dtMs,Field& field,const Player& player,float enemySpeedFactor,LegacyRandom& rng,
                float homingMotionFactor=1.0f);
    const HomingSpecial& homing() const{return homing_;}
    const EraserSpecial& eraser() const{return eraser_;}
    const std::array<EraserDebrisParticle,128>& eraserDebris() const{return eraserDebris_;}
    bool consumeTrailHit(){const bool v=trailHit_;trailHit_=false;return v;}
    bool consumeHomingPlayerHit(float playerX,float playerZ,float& hitX,float& hitY,float& hitZ);
    int consumeHomingSfx22Events(){const int v=homingSfx22Events_;homingSfx22Events_=0;return v;}
    int consumeEraserImpactEvents(){const int v=eraserImpactEvents_;eraserImpactEvents_=0;return v;}
    // 0x4184CD..0x418515: when the eraser speed global (0x254A290) is
    // non-zero, territory capture flood-clears the component containing its
    // current world position in addition to the airborne-enemy components.
    bool eraserCaptureSeed(GridSeed& out) const;
private:
    static int worldToGrid(float v);
    static void chooseEraserDirection(EraserSpecial& e,LegacyRandom& rng);
    static void resetEraserBudget(EraserSpecial& e,LegacyRandom& rng);
    bool eraserBlockTouchesOccupied(const Field& field,int gx,int gy) const;
    void eraseEraserBlock(Field& field,int gx,int gy,LegacyRandom& rng);
    // 0x254A2AC: external one-shot redirect, written only at 0x416227 in the
    // airborne-enemy response path; consumed/reset at 0x415001.
    bool externalRedirect_=false;
public:
    void requestEraserRedirect(){ externalRedirect_=true; }
private:
    void emitEraserDebris(float worldX,float worldZ,LegacyRandom& rng);
    void updateEraserDebris(int dtMs);

    HomingSpecial homing_{};
    EraserSpecial eraser_{};
    bool trailHit_=false;
    int homingSfx22Events_=0; // 0x415621..0x415650: positional logical SFX 0x22, scalar .1.
    int eraserImpactEvents_=0; // 0x4151C7: each event emits exactly 16 debris particles + SFX 0x23.
    std::array<EraserDebrisParticle,128> eraserDebris_{};
    int eraserDebrisHead_=0; // 0x254A2A8; advances in 16-record chunks modulo 128.
};

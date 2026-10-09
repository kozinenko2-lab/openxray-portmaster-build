#include "special_objects.hpp"
#include "player.hpp"
#include <algorithm>
#include <cmath>

namespace {
constexpr float kTwoPi=6.2831853071795864769f;
constexpr float kEraserSpawnBase=.45f;             // 0x43B41C
constexpr float kEraserSpawnStep=.0032258064f;     // 0x43B420
constexpr float kLevelSpeedScale=.000002f;          // 0x43B424
constexpr float kEraserBudgetStep=.0008f;           // 0x43B428
constexpr float kEraserBudgetBase=.04f;             // 0x43B2F4
constexpr float kWorldMin=.41f;                     // 0x43B438
constexpr float kWorldMax=.59f;                     // 0x43B434
constexpr float kGridOrigin=.4015600085f;            // 0x43B430
constexpr float kGridScale=320.f;                   // 0x43B42C = 1/0.003125
constexpr float kHomingFloor=.022f;                 // 0x43B440
constexpr float kHomingDropPerMs=.00004f;            // 0x43B2E0
constexpr float kHomingSpinPerMs=.003f;              // 0x43B380
constexpr float kHomingSoundPhasePerMs=.009f;        // 0x43B3E0
}

int SpecialObjects::worldToGrid(float v){
    // Original uses its x87 float->int helper after (world-0.40156)*320.
    return static_cast<int>((v-kGridOrigin)*kGridScale);
}

bool SpecialObjects::eraserCaptureSeed(GridSeed& out) const{
    // DIRECT EXE 0x4184CD..0x418515. The branch tests the eraser speed
    // scalar itself (0x254A290) against zero, then converts world Z followed
    // by world X through (coord-0x43B430)*0x43B42C and 0x43129C.
    if(eraser_.speed==0.f)return false;
    out.x=worldToGrid(eraser_.worldX);
    out.y=worldToGrid(eraser_.worldZ);
    return true;
}

void SpecialObjects::resetEraserBudget(EraserSpecial& e,LegacyRandom& rng){
    e.travelBudget=float(rng.mask(31))*kEraserBudgetStep+kEraserBudgetBase;
}

void SpecialObjects::chooseEraserDirection(EraserSpecial& e,LegacyRandom& rng){
    e.velocityX=e.velocityZ=0.f;
    switch(rng.mask(3)){
        case 0:e.velocityX=e.speed;break;
        case 1:e.velocityX=-e.speed;break;
        case 2:e.velocityZ=e.speed;break;
        default:e.velocityZ=-e.speed;break;
    }
}

void SpecialObjects::reset(const LevelRecord& level,LegacyRandom& rng){
    trailHit_=false;
    homingSfx22Events_=0;
    eraserImpactEvents_=0;
    for(auto& p:eraserDebris_)p={};

    // r361 DIRECT EXE reset lifetime. 0x415440 rewrites only B0/B4/B8
    // (XYZ), BC (speed) and C0 (the scene motion sign). It does NOT touch
    // C4/C8 (visual/audio phases), CC/D0 (cached direction) or D4 (retarget
    // clock). Those globals are process-lifetime state and therefore survive
    // level changes. Preserve them instead of value-initialising HomingSpecial.
    homing_.active=level.specialHoming!=0;
    homing_.worldX=.5f;homing_.height=.25f;homing_.worldZ=.5f;
    homing_.speed=float(level.specialHoming)*kLevelSpeedScale;

    // 0x414DD0 likewise rewrites 288/28C/290/294/298/29C and clears the
    // 128 debris records, but never writes A0/A4 (presentation phases), A8
    // (16-record debris head) or AC (external redirect latch). Keep those
    // globals alive across a level reset exactly as the EXE does.
    eraser_.active=level.specialEraser!=0;
    eraser_.speed=float(level.specialEraser)*kLevelSpeedScale;
    eraser_.worldX=.5f;
    eraser_.worldZ=.5f;
    eraser_.velocityX=0.f;
    eraser_.velocityZ=0.f;
    eraser_.travelBudget=0.f;
    if(eraser_.active){
        // Preserve the original random-call order from 0x414DD0.
        resetEraserBudget(eraser_,rng);
        eraser_.worldX=float(rng.mask(31))*kEraserSpawnStep+kEraserSpawnBase;
        eraser_.worldZ=float(rng.mask(31))*kEraserSpawnStep+kEraserSpawnBase;
        chooseEraserDirection(eraser_,rng);
    }
}

bool SpecialObjects::eraserBlockTouchesOccupied(const Field& field,int gx,int gy) const{
    // Exact offsets used by 0x415107..0x41515D form x=[-1,+2], y=[-1,+2].
    for(int y=gy-1;y<=gy+2;++y)for(int x=gx-1;x<=gx+2;++x)
        if(field.at(x,y)!=Field::Empty)return true;
    return false;
}

void SpecialObjects::emitEraserDebris(float worldX,float worldZ,LegacyRandom& rng){
    // Exact 0x4151C7..0x415290: the eraser owns an independent 128-record
    // ring buffer. The write head advances by one 16-particle chunk before
    // each impact. Because the head always stays 16-aligned, the 16 writes
    // never cross the end of the 128-record array.
    eraserDebrisHead_=(eraserDebrisHead_+16)&0x7f;
    for(int j=0;j<16;++j){
        auto& p=eraserDebris_[std::size_t(eraserDebrisHead_+j)];
        const int ry=int(rng.mask(63))-32;
        const int rx=int(rng.mask(63))-32;
        const int rz=int(rng.mask(63))-32;
        p.x=worldX; p.y=.008f; p.z=worldZ;
        p.vy=float(ry)*.000002f+.0002f;
        p.vx=float(rx)*.0000002f;
        p.vz=float(rz)*.0000002f;
        p.active=true;
    }
}

void SpecialObjects::updateEraserDebris(int dtMs){
    if(dtMs<=0)return;
    const float dt=float(dtMs);
    // Exact 0x4152CF..0x41531F / sibling 0x414711..0x414764.
    // X/Z and Y integrate using the old velocities; only then is vy stored
    // with gravity applied. The original keeps all 128 records resident and
    // relies on projection rejection once particles fall away.
    constexpr float kGravityPerMs=.00000035f;
    for(auto& p:eraserDebris_){
        // DIRECT EXE 0x4152CF..0x41531F: there is no active test. The full
        // 128-record pool is integrated each frame, including zeroed records.
        const float oldVy=p.vy;
        p.x+=p.vx*dt; p.y+=oldVy*dt; p.z+=p.vz*dt;
        p.vy=oldVy-kGravityPerMs*dt;
    }
}

void SpecialObjects::eraseEraserBlock(Field& field,int gx,int gy,LegacyRandom& rng){
    std::uint8_t any=0;
    for(int y=gy-1;y<=gy+2;++y)for(int x=gx-1;x<=gx+2;++x){
        if(!field.inside(x,y))continue;
        any|=field.at(x,y);
        field.set(x,y,Field::Empty);
    }
    // 0x415199..0x4151B2: bit 0x40 in the OR sets the trail-hazard trigger.
    if(any&Field::Trail)trailHit_=true;
    // 0x4151BC..0x4151C1: debris + SFX 0x23 only when something was erased.
    if(!any)return;
    ++eraserImpactEvents_;
    emitEraserDebris(eraser_.worldX,eraser_.worldZ,rng);
}


bool SpecialObjects::consumeHomingPlayerHit(float playerX,float playerZ,float& hitX,float& hitY,float& hitZ){
    // Exact external consumer 0x419E05..0x419EFD. 0x254A2BC is the
    // homing speed/enable scalar; 0x409140 receives squared radius 1e-4,
    // therefore the direct-body radius is exactly 0.01. The additional
    // height gate requires y < 0.025 before contact can kill Xonix.
    if(!homing_.active || homing_.speed==0.f || !(homing_.height<.025f))return false;
    const float dx=playerX-homing_.worldX;
    const float dz=playerZ-homing_.worldZ;
    if(!(dx*dx+dz*dz<.0001f))return false;
    hitX=homing_.worldX; hitY=homing_.height; hitZ=homing_.worldZ;
    // Literal reset at 0x419E90..0x419EB9 after the death burst.
    homing_.worldX=.5f; homing_.height=.25f; homing_.worldZ=.6f;
    return true;
}

void SpecialObjects::update(int dtMs,Field& field,const Player& player,float enemySpeedFactor,LegacyRandom& rng,
                            float homingMotionFactor){
    if(dtMs<=0)return;
    const float dt=float(dtMs);

    if(homing_.active && homing_.speed>0.f){
        // DIRECT EXE 0x4154CE..0x415558. The masked clock is literal: there is
        // no extra "direction is zero" retarget shortcut.  Crucially, the x87
        // sequence computes factor = 0x257D9FC / distance and stores
        // dx*factor / dz*factor in 0x254A2CC/D0.  Slow/Fast is therefore baked
        // into the cached direction during retarget frames; the later movement
        // multiply does NOT read 0x257D9FC again.  This differs whenever the
        // Because the EXE uses
        // a non-contiguous `& 0x13FF` mask, the reachable no-retarget states are
        // 0x1000..0x13FF; values 0x0400..0x0FFF collapse to the low window.
        homing_.retargetClock=(homing_.retargetClock+dtMs)&0x13ff;
        if(homing_.retargetClock<0x0c00){
            const float dx=player.worldX()-homing_.worldX;
            const float dz=player.worldZ()-homing_.worldZ;
            const float dist=std::sqrt(dx*dx+dz*dz);
            if(dist>1e-8f){
                const float factor=enemySpeedFactor/dist;
                homing_.directionX=dx*factor;
                homing_.directionZ=dz*factor;
            }
        }
        const float homingStep=dt*homingMotionFactor*homing_.speed;
        homing_.worldX+=homingStep*homing_.directionX;
        homing_.worldZ+=homingStep*homing_.directionZ;
        // 0x415560..0x415583 checks the OLD height against .022 and then
        // subtracts dt*.00004 with no post-step clamp. The last descent frame
        // may therefore land slightly below .022 and remains there.
        if(homing_.height>kHomingFloor)
            homing_.height-=dt*kHomingDropPerMs;
        homing_.spinPhase+=dt*kHomingSpinPerMs;
        if(homing_.spinPhase>kTwoPi)homing_.spinPhase-=kTwoPi;
        homing_.soundPhase+=dt*kHomingSoundPhasePerMs;
        // 0x415621..0x415650: strict phase>2pi emits positional logical SFX
        // 0x22 (scalar .1) and subtracts one turn. Runtime dt is clamped, so
        // the original can cross at most once per frame.
        if(homing_.soundPhase>kTwoPi){
            ++homingSfx22Events_;
            homing_.soundPhase-=kTwoPi;
        }
    }

    if(eraser_.active && eraser_.speed>0.f){
        // r194 DIRECT EXE 0x414F4D..0x4151C1 (literal control flow).
        // First candidate and budget use dt*difficulty (0x257D9FC):
        //   x'=x+dt*k*vx, z'=z+dt*k*vz, budget-=dt*k*speed.
        // A candidate is rejected when it leaves [.41,.59], the budget is
        // negative or the external flag 0x254A2AC is set. Rejection clears
        // the flag, draws a new budget and axis, and retries from the SAME
        // start point with dt only (0x41508E..0x4150B2 has no k multiply).
        // The accepted step is always committed. The eraser does NOT stop or
        // turn on filled cells: 0x415107..0x415193 unconditionally clears the
        // 4x4 block, and only the OR of the old bytes decides whether debris,
        // SFX 0x23 and the trail-death trigger (bit 0x40) fire. r4x..r193
        // redirected on every occupied block, so the grader bounced around
        // instead of ploughing straight lines through the captured area.
        const float k=enemySpeedFactor;
        float nx=eraser_.worldX+dt*k*eraser_.velocityX;
        float nz=eraser_.worldZ+dt*k*eraser_.velocityZ;
        eraser_.travelBudget-=dt*k*eraser_.speed;
        // Native safety cap only; the x87 loop is unbounded.
        for(int guard=0;guard<256;++guard){
            const bool valid=!(nx<kWorldMin)&&!(nx>kWorldMax)&&!(nz<kWorldMin)&&!(nz>kWorldMax)
                             &&!(eraser_.travelBudget<0.f)&&!externalRedirect_;
            if(valid)break;
            externalRedirect_=false;
            resetEraserBudget(eraser_,rng);
            chooseEraserDirection(eraser_,rng);
            nx=eraser_.worldX+dt*eraser_.velocityX;
            nz=eraser_.worldZ+dt*eraser_.velocityZ;
        }
        eraser_.worldX=nx;eraser_.worldZ=nz;
        eraseEraserBlock(field,worldToGrid(nx),worldToGrid(nz),rng);
        // 0x41535C..0x415390: independent radial phase in radians.
        eraser_.spinPhase+=dt*.018f; // 0x43B3D8
        while(eraser_.spinPhase>=kTwoPi)eraser_.spinPhase-=kTwoPi;
        // 0x4153A1..0x4153B3: a second legacy-angle accumulator drives
        // the Y rotation. Keep the original unusual 0x7FB mask literally.
        eraser_.rotationAngle2048=(eraser_.rotationAngle2048+dtMs*2)&0x7fb;
    }
    updateEraserDebris(dtMs);
}

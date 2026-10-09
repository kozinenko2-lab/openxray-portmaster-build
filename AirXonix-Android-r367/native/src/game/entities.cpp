#include "entities.hpp"
#include <algorithm>
#include <cmath>
#include "legacy_particles.hpp"

namespace {
constexpr float kPiOver128 = 0.024543693289f;
constexpr float kPiOver8   = 0.392699092627f;
constexpr float kPiOver2   = 1.570796370506f;
constexpr float kTwoPi     = 6.283185307180f;
constexpr float kAngleScale= 325.949323452f; // 2048/(2*pi)
constexpr float kCellStep  = 0.003125f;
constexpr float kWorldBase = 0.4f;
float sq(float x){return x*x;}
}

int Entities::gridRound(float v){
    // x87 FISTP in the original uses round-to-nearest in its normal control mode.
    return static_cast<int>(std::lrintf(v));
}

int Entities::legacyAirHeading2048(float vx,float vy){
    // r364 DIRECT EXE 0x415B18..0x415B4C. The native helper does not normalize
    // with atan2() and does not mask here. It computes atan(vy/vx), adds the
    // single-precision PI constant only when vx<0, multiplies by 2048/(2*PI),
    // then converts toward zero through the VC6 _ftol helper. Consequently
    // quadrant-IV headings remain negative until the matrix helper masks them.
    float a=std::atan(vy/vx);
    if(vx<0.f)a+=3.1415927410125732f;
    return static_cast<int>(a*kAngleScale);
}

bool Entities::airNearOccupied(const Field& f,int x,int y,bool* touchesTrail){
    static constexpr int dx[8]={1,-1,0,0,1,-1,1,-1};
    static constexpr int dy[8]={0,0,1,-1,1,-1,-1,1};
    std::uint8_t bits=0;
    for(int i=0;i<8;++i) bits|=f.at(x+dx[i],y+dy[i]);
    if(touchesTrail) *touchesTrail=(bits&Field::Trail)!=0;
    return bits!=0;
}

void Entities::exchangeApproachingComponents(float ax,float ay,float bx,float by,
                                               float& avx,float& avy,float& bvx,float& bvy){
    // 0x416110 resolves pair contacts component-by-component.  For each axis,
    // an approaching pair transfers the velocity difference, equivalent to a
    // component swap for equal-mass entities.
    if(ax<bx){ const float d=avx-bvx; if(d>0.f){avx-=d;bvx+=d;} }
    else if(ax>bx){ const float d=bvx-avx; if(d>0.f){bvx-=d;avx+=d;} }
    if(ay<by){ const float d=avy-bvy; if(d>0.f){avy-=d;bvy+=d;} }
    else if(ay>by){ const float d=bvy-avy; if(d>0.f){bvy-=d;avy+=d;} }
}

void Entities::emitFieldDebris(int gridX,int gridY,LegacyRandom& rng){
    // 0x4175B0: scan the shared 96-record pool and initialize at most 16
    // records whose y is below the free-slot threshold 0.005. The caller
    // supplies one grid location for each eroded neighbour.
    int emitted=0;
    const float wx=float(gridX)*kCellStep+kWorldBase;
    const float wz=float(gridY)*kCellStep+kWorldBase;
    for(auto& p:fieldDebris_){
        if(p.active()) continue;
        const auto v=LegacyParticles::fieldVelocity(rng);
        p.x=wx; p.y=LegacyParticles::FieldSpawnY; p.z=wz;
        p.vx=v.x; p.vy=v.y; p.vz=v.z;
        if(++emitted>=LegacyParticles::FieldEmitMax) break;
    }
}

void Entities::updateFieldDebris(int dtMs){
    if(dtMs<=0) return;
    const float dt=float(dtMs);
    for(auto& p:fieldDebris_){
        // r365 DIRECT EXE 0x417760..0x4177A5: the renderer/update walks every
        // one of the 96 records. A negative Y is snapped to -0.5 and skips the
        // integration for this frame. Non-negative records integrate even when
        // Y<=0.005 (that threshold belongs only to 0x4175B0 slot allocation).
        if(p.y<0.f){p.y=-0.5f;continue;}
        const float oldVy=p.vy;
        p.x+=p.vx*dt; p.y+=oldVy*dt; p.z+=p.vz*dt;
        p.vy=oldVy-dt*LegacyParticles::FieldGravityPerMs;
    }
}

void Entities::erodeAirImpact(Field& f,int x,int y,LegacyRandom& rng){
    x=std::clamp(x,2,61); y=std::clamp(y,2,61);
    ++airErosionImpactEvents_;
    static constexpr int dx[8]={1,-1,0,0,1,-1,1,-1};
    static constexpr int dy[8]={0,0,1,-1,1,-1,-1,1};
    for(int i=0;i<8;++i){
        const int px=x+dx[i],py=y+dy[i];
        // DIRECT EXE 0x415FA0: all eight neighbours are cleared, but their
        // pre-clear bytes are saved first. 0x4175B0 is then called only for
        // neighbours whose saved byte was non-zero. The old reconstruction
        // emitted eight debris helpers unconditionally and visibly overfilled
        // already-empty impacts.
        const auto old=f.at(px,py);
        f.set(px,py,Field::Empty);
        if(old!=Field::Empty){
            ++airErosionDebrisHelperCalls_;
            emitFieldDebris(px,py,rng);
        }
    }
}

void Entities::reset(const LevelRecord& r,const Field& field,LegacyRandom& rng){
    // r363 DIRECT EXE 0x415BA0: the 80-byte airborne records live in a static
    // array. Level initialization rewrites +0..+0x1c and the transform blob at
    // +0x28, but never touches cached worldX/worldZ at +0x20/+0x24. Preserve
    // those two floats for slots that existed on the previous level; slots
    // never used before retain their BSS-zero values. The first 0x416110 update
    // rewrites the caches at 0x416725..0x416773.
    std::vector<std::pair<float,float>> oldAirWorld;
    oldAirWorld.reserve(air_.size());
    for(const auto& e:air_) oldAirWorld.emplace_back(e.worldX,e.worldZ);
    air_.clear(); ground_.clear(); trailHit_=false; trailImpacts_.clear();
    // r365 DIRECT EXE 0x4190B4..0x4190C7: level init writes -1.0 only into
    // the Y field of the first 64 (not 96) shared 24-byte debris records. The
    // other fields and the final 32 records retain process-lifetime contents.
    for(std::size_t i=0;i<64u;++i) fieldDebris_[i].y=-1.0f;
    airErosionImpactEvents_=0; airErosionDebrisHelperCalls_=0; collisionSfxEvents_.clear(); crawlerCollisionStampValid_=false;
    const float speed=static_cast<float>(r.enemySpeed)*0.001f*2.0f;
    const int total=r.enemyTypeACount+r.enemyTypeBCount;
    air_.reserve(static_cast<std::size_t>(total));

    for(int i=0;i<total;++i){
        AirEnemy e;
        const float angle=static_cast<float>(rng.mask(31))*kPiOver128+kPiOver8+
                          static_cast<float>(rng.mask(3))*kPiOver2;
        e.vx=std::cos(angle)*speed; e.vy=std::sin(angle)*speed;

        for(;;){
            const int x=rng.mask(63), y=rng.mask(63);
            if(x<4||x>=60||y<4||y>=60) continue;
            bool reject=false;
            // Original scans every occupied interior cell and keeps >=4 cells away.
            for(int fy=1;fy<63&&!reject;++fy) for(int fx=1;fx<63;++fx){
                if(field.at(fx,fy)!=Field::Empty && sq(float(fx-x))+sq(float(fy-y))<16.f){reject=true;break;}
            }
            if(reject) continue;
            for(const auto& o:air_) if(sq(o.x-float(x))+sq(o.y-float(y))<25.f){reject=true;break;}
            if(reject) continue;
            e.x=static_cast<float>(x);e.y=static_cast<float>(y);break;
        }

        // The original consumes two rand() calls and overwrites the first result.
        // Preserve that seemingly redundant call so the global random stream matches.
        (void)rng.mask(1);
        const int subtypeBit=rng.mask(1);
        e.subtype=(i<r.enemyTypeACount)?subtypeBit:(2+subtypeBit);
        e.heading2048=legacyAirHeading2048(e.vx,e.vy);
        e.spinStep=std::max(1,static_cast<int>(std::sqrt(e.vx*e.vx+e.vy*e.vy)*125.f));
        e.angularAccumulator=0;
        const std::size_t slot=air_.size();
        if(slot<oldAirWorld.size()){
            e.worldX=oldAirWorld[slot].first;
            e.worldZ=oldAirWorld[slot].second;
        }
        // New slots intentionally keep BSS-zero cached coordinates.
        air_.push_back(e);
    }

    const float groundSpeed=static_cast<float>(r.crawlerSpeed)*0.001f*2.0f;
    ground_.reserve(r.crawlerCount);
    int alternating=1;
    for(int i=0;i<r.crawlerCount;++i){
        GroundEnemy e;
        e.x=32.f;e.y=65.f;
        alternating=-alternating; // original negates before using the sign
        const float angle=(static_cast<float>(rng.mask(31))-15.f)*0.02f+kPiOver8*2.f; // pi/4
        e.vx=std::cos(angle)*float(alternating)*groundSpeed;
        e.vy=std::sin(angle)*groundSpeed;
        e.respawnDelay=0.5f+0.2f*static_cast<float>(i);
        e.worldX=1.f;e.worldZ=0.f;
        ground_.push_back(e);
    }
}

bool Entities::groundBlocked(const Field& f,int x,int y) const {
    // 0x4168E0 returns 1 for blocked and 0 for allowed.
    if(x<-4||x>67||y<-4||y>67) return true;
    if(x>0&&x<63&&y>0&&y<63) return f.at(x,y)!=Field::Safe;
    return false;
}

void Entities::resetGroundEnemy(std::size_t index,GroundEnemy& e){
    e.x=32.f;e.y=65.f;e.respawnDelay=0.1f+0.2f*static_cast<float>(index);
    e.worldX=e.x*kCellStep+kWorldBase;e.worldZ=e.y*kCellStep+kWorldBase;
}

void Entities::resetCrawlerTransition(){
    // Literal 0x416890 contract: only record +0, +4 and +0x10 are written.
    // In particular the cached world coordinates (+0x14/+0x18 in the original)
    // are intentionally left untouched until the crawler update activates it.
    for(std::size_t i=0;i<ground_.size();++i){
        auto& e=ground_[i];
        e.x=32.f;
        e.y=65.f;
        e.respawnDelay=0.1f+0.2f*static_cast<float>(i);
    }
}

void Entities::updateAirOnly(int dtMs,Field& field,LegacyRandom& rng,float enemySpeedFactor){
    if(dtMs<=0)return;
    // 0x416110 resolves air-enemy pair contacts once per outer update.
    for(std::size_t i=0;i<air_.size();++i)for(std::size_t j=i+1;j<air_.size();++j){
        auto& a=air_[i];auto& b=air_[j];
        if(!a.active||!b.active)continue;
        if(sq(a.x-b.x)+sq(a.y-b.y)<9.f){
            exchangeApproachingComponents(a.x,a.y,b.x,b.y,a.vx,a.vy,b.vx,b.vy);
            // DIRECT EXE 0x416486..0x4164AC: every airborne pair contact
            // submits positional logical SFX 3 / bol2 at the first record's
            // cached world X/Z, Y=.004, scalar 1.0. The call is unconditional
            // once the squared grid distance is below 9; there is no cooldown.
            collisionSfxEvents_.push_back({0x03u,a.worldX,0.004000000189989805f,a.worldZ,1.0f});
        }
    }

    for(auto& e:air_){
        if(!e.active)continue;

        // Original 0x416110 advances an integer accumulator 0,2,4,... while it is
        // below dt. e.vx/e.vy were doubled at spawn, so each loop is one legacy
        // two-time-unit movement quantum. 0x257D9FC is a separate enemy-only
        // speed factor (normally 1.0; bonus type 3 sets it to 0.3).
        for(int sub=0;sub<dtMs;sub+=2){
            const float oldX=e.x,oldY=e.y;
            float nx=oldX+e.vx*enemySpeedFactor;
            float ny=oldY+e.vy*enemySpeedFactor;
            const int ox=gridRound(oldX),oy=gridRound(oldY);
            const int cx=gridRound(nx),cy=gridRound(ny);
            bool trail=false;
            if(airNearOccupied(field,cx,cy,&trail)){
                // DIRECT EXE 0x4165C1..0x4165E7: occupied-field impact first
                // submits positional logical SFX 2 / bol1 using the airborne
                // record's cached world X/Z, Y=.004, scalar 1.0. This occurs
                // once per colliding two-time-unit movement quantum.
                collisionSfxEvents_.push_back({0x02u,e.worldX,0.004000000189989805f,e.worldZ,1.0f});
                // 0x415F10/0x4165A1..0x4165BE: when the OR of the eight
                // neighbouring field bytes contains Trail(0x40), the EXE
                // publishes the sampled grid center and a one-shot hazard flag.
                if(trail){trailHit_=true;trailImpacts_.push_back({cx,cy});}
                const bool xBlocked=airNearOccupied(field,cx,oy,nullptr);
                const bool yBlocked=airNearOccupied(field,ox,cy,nullptr);
                if(cx!=ox&&cy!=oy){
                    if(xBlocked&&!yBlocked)e.vx=-e.vx;
                    else if(!xBlocked&&yBlocked)e.vy=-e.vy;
                    else {e.vx=-e.vx;e.vy=-e.vy;}
                }else if(cx!=ox)e.vx=-e.vx;
                else if(cy!=oy)e.vy=-e.vy;
                else {e.vx=-e.vx;e.vy=-e.vy;}

                // DIRECT EXE 0x4166CE..0x4166FC: every airborne subtype
                // calls 0x415FA0 after an occupied-neighbour collision. The
                // subtype changes the erosion centre: 2/3 use the rounded
                // candidate position, while 0/1 use the rounded pre-step
                // position. This is not a destructive-vs-nondestructive split.
                erodeAirImpact(field,e.subtype>1?cx:ox,e.subtype>1?cy:oy,rng);
                nx=oldX+e.vx*enemySpeedFactor;
                ny=oldY+e.vy*enemySpeedFactor;
            }
            e.x=nx;e.y=ny;
        }

        e.worldX=e.x*kCellStep+kWorldBase;e.worldZ=e.y*kCellStep+kWorldBase;
        // 0x41677F..0x4167B5: angle -= spinStep * dt * 0x257D9FC.
        // Slow-enemy bonus therefore slows both translation and visible ball rotation.
        e.angularAccumulator-=static_cast<int>(float(e.spinStep*dtMs)*enemySpeedFactor);
        e.heading2048=legacyAirHeading2048(e.vx,e.vy);
    }

}

void Entities::update(int dtMs,Field& field,LegacyRandom& rng,float enemySpeedFactor,
                      bool deathRaiseCrawlers,bool updateDebris){
    if(dtMs<=0)return;
    updateAirOnly(dtMs,field,rng,enemySpeedFactor);
    const float dt=static_cast<float>(dtMs);
    crawlerCollisionClockMs_+=static_cast<std::uint64_t>(dtMs);
    // 0x41C821..0x41C854: after the death impact phase becomes non-zero the
    // normal 0x416950 crawler updater is skipped. Every crawler +0x10 field is
    // instead raised by dt*0.00015, producing the original upward "jump".
    if(deathRaiseCrawlers){
        for(auto& e:ground_)if(e.active)e.respawnDelay+=dt*0.0001500000071246177f;
        if(updateDebris)updateFieldDebris(dtMs);
        return;
    }

    // Ground/crawler pair contacts are also resolved before the 2-unit movement
    // loop. A non-zero +0x10 field is a spawn/reset delay, not a render phase.
    for(std::size_t i=0;i<ground_.size();++i){
        auto& e=ground_[i];if(!e.active)continue;
        if(e.respawnDelay>0.f){
            e.respawnDelay-=dt*0.0001f;
            if(e.respawnDelay>0.f)continue;
            e.respawnDelay=0.f;
        }
        for(std::size_t j=i+1;j<ground_.size();++j){
            auto& b=ground_[j];if(!b.active||b.respawnDelay!=0.f)continue;
            if(sq(e.x-b.x)+sq(e.y-b.y)<6.f){
                exchangeApproachingComponents(e.x,e.y,b.x,b.y,e.vx,e.vy,b.vx,b.vy);
                // DIRECT EXE 0x416C38..0x416C80: crawler pair-contact audio is
                // globally debounced. timeGetTime()-last must be strictly >50
                // ms to play logical SFX 1 / min0, at the outer crawler's
                // cached X/Z, Y=.009, scalar 1.0. Every contact refreshes last,
                // even when the sound is suppressed.
                const bool play=!crawlerCollisionStampValid_ ||
                    (crawlerCollisionClockMs_-crawlerLastCollisionMs_>50u);
                if(play) collisionSfxEvents_.push_back({0x01u,e.worldX,0.008999999612569809f,e.worldZ,1.0f});
                crawlerLastCollisionMs_=crawlerCollisionClockMs_;
                crawlerCollisionStampValid_=true;
            }
        }

        bool reset=false;
        for(int sub=0;sub<dtMs;sub+=2){
            const int ox=gridRound(e.x),oy=gridRound(e.y);
            float nx=e.x+e.vx*enemySpeedFactor;
            float ny=e.y+e.vy*enemySpeedFactor;
            int cx=gridRound(nx),cy=gridRound(ny);
            if(groundBlocked(field,cx,cy)){
                // DIRECT EXE 0x416DFE..0x416E46: crawler field-contact shares
                // the same 50-ms debounce stamp as crawler pair contacts. It
                // plays logical SFX 0 / min0 at cached X/Z, Y=.009, scalar .5,
                // and refreshes the timestamp whether or not playback occurs.
                const bool play=!crawlerCollisionStampValid_ ||
                    (crawlerCollisionClockMs_-crawlerLastCollisionMs_>50u);
                if(play) collisionSfxEvents_.push_back({0x00u,e.worldX,0.008999999612569809f,e.worldZ,0.5f});
                crawlerLastCollisionMs_=crawlerCollisionClockMs_;
                crawlerCollisionStampValid_=true;
                if(field.inside(cx,cy) && (field.at(cx,cy)&Field::Trail)){
                    trailHit_=true; trailImpacts_.push_back({cx,cy});
                }
                const bool xb=groundBlocked(field,cx,oy),yb=groundBlocked(field,ox,cy);
                if(cx!=ox&&cy!=oy){
                    if(xb&&!yb)e.vx=-e.vx;
                    else if(!xb&&yb)e.vy=-e.vy;
                    else{e.vx=-e.vx;e.vy=-e.vy;}
                }else if(cx!=ox)e.vx=-e.vx;
                else if(cy!=oy)e.vy=-e.vy;
                else{e.vx=-e.vx;e.vy=-e.vy;}
                nx=e.x+e.vx*enemySpeedFactor;
                ny=e.y+e.vy*enemySpeedFactor;
                cx=gridRound(nx);cy=gridRound(ny);
            }
            if(groundBlocked(field,cx,cy)){resetGroundEnemy(i,e);reset=true;break;}
            e.x=nx;e.y=ny;
        }
        if(!reset){e.worldX=e.x*kCellStep+kWorldBase;e.worldZ=e.y*kCellStep+kWorldBase;}
    }

    // 0x417700 is a separate legacy call, not part of 0x416110/0x416950.
    // Most world states call it every frame, but inter-level gates it to
    // remaining>2000 and finale never reaches its peculiar boolean>2000 gate.
    if(updateDebris)updateFieldDebris(dtMs);
}

std::vector<GridSeed> Entities::captureSeeds() const {
    std::vector<GridSeed> s;s.reserve(air_.size());
    for(const auto& e:air_)if(e.active)s.push_back({static_cast<int>(e.x),static_cast<int>(e.y)});
    return s;
}

bool Entities::resolveEraserAirContacts(float eraserWorldX,float eraserWorldZ){
    // DIRECT EXE 0x41612E..0x41623A. 0x43B350 is 1e-4, so the contact radius
    // is exactly .01.  The original x87 expression reduces independently to
    // abs(vx)*sign(dx) and abs(vy)*sign(dz), then writes 0x254A2AC=1 and calls
    // 0x415AD0 to rebuild the enemy's orientation metadata.  spinStep depends
    // only on speed magnitude and our heading is rebuilt after movement, so no
    // extra native-only transform state is required here.
    constexpr float kRadiusSq=9.999999747378752e-5f;
    bool redirect=false;
    for(auto& e:air_){
        if(!e.active)continue;
        const float dx=e.worldX-eraserWorldX;
        const float dz=e.worldZ-eraserWorldZ;
        if(dx*dx+dz*dz>=kRadiusSq)continue;
        if(dx>0.f)e.vx=std::fabs(e.vx);
        else if(dx<0.f)e.vx=-std::fabs(e.vx);
        if(dz>0.f)e.vy=std::fabs(e.vy);
        else if(dz<0.f)e.vy=-std::fabs(e.vy);
        redirect=true;
    }
    return redirect;
}



bool Entities::consumeCrawlerPlayerHit(float worldX,float worldZ,float& hitX,float& hitZ){
    // 0x419D0A..0x419DDB: exact crawler -> Xonix collision consumer lives in
    // the main gameplay loop, not in 0x416950. 0x409140 compares squared
    // world-space distance against 4.225e-5 (= 0.0065^2), and the crawler is
    // lethal only when +0x10 respawnDelay is below 0.015. On hit the original
    // immediately moves that crawler back to (32,65) and stores delay 1.7.
    constexpr float kHitRadiusSq=4.225000157020986e-5f;
    constexpr float kActiveDelayLimit=0.015f;
    for(std::size_t i=0;i<ground_.size();++i){
        auto& e=ground_[i];
        if(!e.active || !(e.respawnDelay<kActiveDelayLimit)) continue;
        const float dx=worldX-e.worldX,dz=worldZ-e.worldZ;
        if(dx*dx+dz*dz>=kHitRadiusSq) continue;
        hitX=e.worldX; hitZ=e.worldZ;
        e.x=32.f; e.y=65.f; e.respawnDelay=1.7f;
        e.worldX=e.x*kCellStep+kWorldBase; e.worldZ=e.y*kCellStep+kWorldBase;
        return true;
    }
    return false;
}

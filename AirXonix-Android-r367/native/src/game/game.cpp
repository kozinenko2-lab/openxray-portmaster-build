#include "game.hpp"
#include "game/legacy_records_transition.hpp"
#include "legacy_controls_trace.hpp"
#include "legacy_restore_trace.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include "legacy_particles.hpp"
#include "legacy_information_transition.hpp"
#include "render/legacy_theme.hpp"
#include "render/legacy_background_scroll.hpp"
#include "audio/legacy_audio_trace.hpp"

namespace {
constexpr float kPi=3.14159265358979323846f;
constexpr float kTwoPi=6.2831853071795864769f;
constexpr int kFinalInputArmMs=7000;    // 0x1B58, 0x41B4EF
constexpr int kFinalAutoExitMs=90000;   // 0x15F90, 0x41B548
// DIRECT EXE CONFIRMED by the r35 CFG trace: 0x41C081 stores 0xFA0,
// 0x41C1A7 decrements the same countdown, and the negative path reaches
// normal cleanup at 0x41CE2C.
constexpr int kDeathPresentationMs=4000;
// 0x41C6FE compares the already-negative countdown against 0xFFFF15A0
// (-60000): zero-lives presentation may remain active for one extra minute.
constexpr int kGameOverTailMs=60000;
// CONFIRMED by direct AirXonix.wrp.exe trace: 0x0041A9FF executes
// MOV ESI,0xFA0 and 0x0041A980 exits after that countdown becomes negative.
constexpr int kInterLevelPresentationMs=4000;

template<class F> struct ScopeExit { F f; ~ScopeExit(){ f(); } };
template<class F> ScopeExit(F)->ScopeExit<F>;

inline int legacyTruncAdd(int score,float term){
    return static_cast<int>(static_cast<float>(score)+term);
}

DeathAudioVoiceTag pickupVoiceTag(std::size_t slot){
    static constexpr DeathAudioVoiceTag tags[6]={DeathAudioVoiceTag::Pickup0,DeathAudioVoiceTag::Pickup1,DeathAudioVoiceTag::Pickup2,DeathAudioVoiceTag::Pickup3,DeathAudioVoiceTag::Pickup4,DeathAudioVoiceTag::Pickup5};
    return tags[slot<6?slot:0];
}

}

Game::Game(){ startNewSession(0); }
Game::Game(const std::vector<std::uint8_t>& soundInf):database_(soundInf){ startNewSession(0); }

void Game::showMainMenuOnBoot(){
    // r302 DIRECT EXE 0x424D09..0x424DA8 precedes gameplay wrapper 0x424DE0.
    // The native Game constructor preloads a harmless level for object validity,
    // but that must not leak its RNG/selector side effects into the real cold
    // boot. Reset the shared MSVC stream and environment usage before the first
    // M1 constructor; enterMainMenu then consumes exactly the original first
    // menu-theme selector rand().
    rng_.seed(1u);
    menuThemeUsage_.fill(0u);
    environmentThemeUsage_.fill(0u);
    airxonix::LegacyMusicSelectorTrace::reset(musicTrackUsage_);
    enterMainMenu();
}


bool Game::initializeLegacySettings(const std::filesystem::path& path){
    settingsPath_=path;
    struct RawSettings { float speed,sfx,music; std::int32_t reserved; std::int32_t keys[4]; std::int32_t speech; };
    static_assert(sizeof(RawSettings)==LegacySettingsTrace::fileSize);
    RawSettings raw{};
    std::ifstream f(path,std::ios::binary);
    if(f){
        f.read(reinterpret_cast<char*>(&raw),sizeof(raw));
        if(f.gcount()==static_cast<std::streamsize>(sizeof(raw)) && f.peek()==std::char_traits<char>::eof()){
            settings_.speed=raw.speed; settings_.sfx=raw.sfx; settings_.music=raw.music;
            legacySelectedMode_=raw.reserved;
            for(int i=0;i<4;++i)settings_.bindings[std::size_t(i)]=raw.keys[i];
            settings_.speech=raw.speech!=0; speechEnabled_=settings_.speech;
            return true;
        }
    }
    // 0x423040 exact 36-byte defaults used after any failed/short read.
    settings_.speed=800.f; settings_.sfx=1000.f; settings_.music=800.f;
    legacySelectedMode_=0;
    settings_.bindings={{0x41,0x5A,0x58,0x43}};
    settings_.speech=true; speechEnabled_=true;
    return saveLegacySettingsNow();
}

bool Game::saveLegacySettingsNow() const{
    if(settingsPath_.empty())return false;
    struct RawSettings { float speed,sfx,music; std::int32_t reserved; std::int32_t keys[4]; std::int32_t speech; };
    static_assert(sizeof(RawSettings)==LegacySettingsTrace::fileSize);
    RawSettings raw{settings_.speed,settings_.sfx,settings_.music,legacySelectedMode_,{},settings_.speech?1:0};
    for(int i=0;i<4;++i)raw.keys[i]=settings_.bindings[std::size_t(i)];
    std::error_code ec;
    if(const auto parent=settingsPath_.parent_path();!parent.empty())std::filesystem::create_directories(parent,ec);
    std::ofstream f(settingsPath_,std::ios::binary|std::ios::trunc);
    if(!f)return false;
    f.write(reinterpret_cast<const char*>(&raw),sizeof(raw));
    return bool(f);
}

bool Game::initializeLegacyHighScores(const std::filesystem::path& path){
    // r239 DIRECT EXE 0x40F010: one strict 0x640-byte read. Any failure,
    // including a short file, rebuilds all eight blocks and immediately writes
    // the complete binary image back through 0x40EFF0 semantics.
    highScorePath_=path;
    highScores_=airxonix::makeLegacyDefaultHighScores();
    if(airxonix::loadLegacyHighScores(path,highScores_))return true;
    highScores_=airxonix::makeLegacyDefaultHighScores();
    return airxonix::saveLegacyHighScores(path,highScores_);
}

bool Game::saveLegacyHighScoresNow() const{
    return !highScorePath_.empty() && airxonix::saveLegacyHighScores(highScorePath_,highScores_);
}

void Game::startNewSession(std::size_t mode){
    // 0x418DB0: session bootstrap restores the legacy spatial-listener basis.
    legacyAudioBasisAngle2_=-302;
    // 0x418D10 is a campaign/session bootstrap, distinct from the per-level
    // initializer at 0x418DD0. The original clears campaign-owned counters
    // only here; deaths and inter-level transitions must keep them.
    mode_=mode;
    level_=0;
    lives_=settings_.testInitialLives;
    score_=0;
    scoreDecayAccumulatorMs_=0;
    lowTimeWarningActive_=false;
    bonusAccumulator_=0;
    // 0x424DFA: one campaign owns exactly five current-level restart credits.
    restartCredits_=kLegacyRestartCurrentLevelTrace.initialCredits;
    restartCheckpointLevel_=0;
    restartCheckpointScore_=0;
    restartCheckpointLives_=lives_;
    difficultyScale_=legacyDifficultyScale(settings_.speed); // 0x424E2D -> 0x257D9E8
    // 0x424E24 -> 0x422EA0: a new campaign resets the exact music selector
    // counters to {1,0,0,0,0,0,0,0,0,0}.
    airxonix::LegacyMusicSelectorTrace::reset(musicTrackUsage_);
    player_.reset(); // r266 session bootstrap owns DA40/DADC initialization.
    loadLevel(mode_,0);
    // DIRECT EXE 0x418D11 -> 0x4156A0: session bootstrap only writes Y=.5
    // to each of the six static auxiliary records. X/Z/trigger survive, but
    // Y=.5 makes every record inactive until a later 0x4156C0 spawn overwrites
    // X/Y/Z/trigger. Do not zero the whole C++ struct here.
    for(auto& a:auxiliaryEffects_)a.y=0.5f;
}

void Game::loadLevel(std::size_t mode,std::size_t level){
    ++levelLoadSerial_;
    mode_=mode;level_=level;phase_=GamePhase::Gameplay;levelIntroScene_={};levelEntryScene_={};deathScene_={};deathOverlay_={};gameOverScene_={};interLevelScene_={};finalScene_={};effectEvents_={};levelMusicPending_=true;
    // r291 DIRECT EXE: outer wrapper 0x424EB9 calls 0x423350, whose first
    // operation is environment selector 0x422FC0, before per-level init
    // 0x418DD0. Preserve that RNG consumption before field/entities/pickups.
    environmentThemeIndex_=kLegacyEnvironmentThemeSelectorTrace.select(environmentThemeUsage_,rng_.next());
    const LevelRecord& r=database_.level(mode_,level_);
    field_.build(r);
    player_.resetForLevel();
    entities_.reset(r,field_,rng_);
    scoreMultiplier_=legacyScoreMultiplier(r,difficultyScale_);
    initialOccupied_=field_.nonzeroCount();lastOccupied_=initialOccupied_;
    // r267: DAEC/DAF0 snapshot score/lives at level entry for Backspace restore.
    restartCheckpointLevel_=level_; restartCheckpointScore_=score_; restartCheckpointLives_=lives_;
    captureDenominator_=4096-initialOccupied_-100*r.enemyTypeACount-120*r.enemyTypeBCount-(r.specialEraser?100:0);
    if(captureDenominator_<1)captureDenominator_=1;
    capturePercent_=0;timer_=settings_.testInitialTimeSeconds<<10;bonusAccumulator_=0;
    hudDisplayTimer_=timer_; hudDisplayScore_=score_; hudPulseCounter_=0;

    enemySpeedFactor_=1.0f;
    playerMaxSpeed_=0.03f;
    playerRotorPhase_=0.0f;
    playerRotorRadius_=0.0047f;
    playerPropellerPhase_=0.0f;
    playerPropellerStep_=0.0f;
    legacyCameraZoom_=0.0f;
    legacyCameraZoomRate_=0.0f;
    brightnessScale_=1.0f;
    brightnessRecoveryRate_=0.00018f;
    legacyOscAmplitude_=0.0f;
    legacyOscPhase_=0.0f;
    legacyCameraYawOffset_=0;
    player_.setMaxSpeed(playerMaxSpeed_);

    // In the original level initializer, the six pickup respawns happen before
    // 0x415440 (homing special) and 0x414DD0 (eraser special). Preserve this
    // ordering because all systems share the same MSVC rand() stream.
    pickups_.reset(field_,rng_,timer_);
    specialObjects_.reset(r,rng_);
}


void Game::restartCurrentLevel(){
    if(phase_!=GamePhase::Gameplay || restartCredits_<=0)return;

    // r185 DIRECT EXE: 0x4192A3 stores currentLevel-1 plus score/lives, then
    // 0x424F2F loops through the restore guard at 0x424E2D. In the native
    // zero-based representation currentLevel-1 is exactly level_. 0x419250
    // restores the three checkpoint fields before 0x418DD0 reinitializes the
    // same level. The restore consumes one of the five campaign credits.
    // Match the cleanup that the original gameplay/ABOR path performs before
    // control returns to the campaign wrapper: retained pickup/tick handles
    // cannot survive into the freshly initialized level.
    for(std::size_t i=0;i<6;++i)queueDeathSpatialStop(pickupVoiceTag(i));
    if(lowTimeWarningActive_){
        queueDeathSpatialStop(DeathAudioVoiceTag::LowTimeWarning);
        lowTimeWarningActive_=false;
    }

    --restartCredits_;
    const std::size_t restoredLevel=restartCheckpointLevel_;
    score_=restartCheckpointScore_;
    lives_=restartCheckpointLives_;
    loadLevel(mode_,restoredLevel);
}

void Game::selectLevelMusic(){
    if(!levelMusicPending_)return;
    // r291 DIRECT EXE: 0x41CEA0 returns from the 3000-ms level intro first,
    // then 0x41CEE1 calls music selector 0x422EE0 and 0x41CEE7 queues it.
    currentMusicTrack_=airxonix::LegacyMusicSelectorTrace::select(musicTrackUsage_,rng_.next());
    queueMusicRequest(currentMusicTrack_,0.0005000000237487257f);
    std::fprintf(stderr,"AX_PRESENTATION mode=%zu level=%zu env_theme=%zu music=%02zu\n",
                 mode_,level_,environmentThemeIndex_,currentMusicTrack_);
    levelMusicPending_=false;
}

void Game::updateScoreAndPercent(){
    const int occupied=field_.occupiedMaskedCount();
    int delta=occupied-lastOccupied_;lastOccupied_=occupied;bonusAccumulator_=std::max(0,bonusAccumulator_+delta);
    score_=std::max(0,static_cast<int>(static_cast<float>(score_)+static_cast<float>(delta)*scoreMultiplier_));
    const int captured=std::max(0,occupied-initialOccupied_);
    capturePercent_=std::max(0,(captured*100)/captureDenominator_);
    // 0x419568..0x419597: the accumulated newly-captured-cell counter is
    // compared with lives*400. Crossing the threshold resets the accumulator
    // and clamps pickup slot #2's hidden delay to at most 1000 ms, making an
    // extra-life pickup available soon without relocating it yet.
    if(bonusAccumulator_>lives_*400){
        bonusAccumulator_=0;
        pickups_.forceExtraLifePickupSoon();
    }
}


void Game::updateLegacyTimerPressure(int dtMs){
    // 0x4196EE..0x4197D8: retained low-time warning. The voice starts once
    // timer drops below 0x2AF8, follows the current camera listener basis,
    // and is stopped again if a time bonus raises the timer above threshold.
    constexpr int kLowTimeThreshold=0x2AF8;
    constexpr float kLowTimeTimerToZ=1.9999999494757503e-5f; // 0x43B4B4
    const float camX=(player_.worldX()-.4f)*.5f+.45f;
    const float camY=.103f-1.1f*legacyCameraZoom_;
    const float camZ=(player_.worldZ()-.4f)*.5f+.35f+legacyCameraZoom_;
    if(timer_<kLowTimeThreshold){
        if(!lowTimeWarningActive_){
            queueDeathSpatialStart(DeathAudioVoiceTag::LowTimeWarning,0x12u,0.f,0.f,0.f,1.f);
            lowTimeWarningActive_=true;
        }
        queueDeathSpatialUpdate(DeathAudioVoiceTag::LowTimeWarning,camX,camY,camZ+0.01f+float(timer_)*kLowTimeTimerToZ);
    }else if(lowTimeWarningActive_){
        queueDeathSpatialStop(DeathAudioVoiceTag::LowTimeWarning);
        lowTimeWarningActive_=false;
    }

    // 0x4197D8..0x419807: strict >1000 test, subtract 50, clamp at zero.
    scoreDecayAccumulatorMs_+=dtMs;
    while(scoreDecayAccumulatorMs_>1000){
        scoreDecayAccumulatorMs_-=1000;
        score_=std::max(0,score_-50);
    }
}

void Game::spawnAuxiliaryEffect(int slot,float x,float y,float z){
    if(slot<0||slot>=int(auxiliaryEffects_.size()))return;
    auto& a=auxiliaryEffects_[std::size_t(slot)];
    a.x=x;
    a.y=y+0.01f; // 0x415804 / 0x43B330
    a.z=z;
    a.trigger=true;
}

void Game::updateAuxiliaryEffects(int dtMs){
    if(dtMs<=0)return;
    static constexpr std::array<std::size_t,5> kSpeech{{0x21u,0x24u,0x20u,0x25u,0x26u}};
    static constexpr std::array<float,5> kScalar{{1.2f,1.1f,1.4f,1.0f,1.0f}};
    const float dt=float(dtMs);
    for(std::size_t i=0;i<5;++i){
        auto& a=auxiliaryEffects_[i];
        if(!a.active())continue;
        // 0x4158B5..0x415996 checks the armed flag and threshold before the
        // per-frame Y increment. Speech-disabled frames intentionally leave
        // trigger armed.
        if(a.trigger && a.y>0.03f && speechEnabled_){
            queueDeathSpatialPlay(kSpeech[i],a.x,a.y,a.z,kScalar[i]);
            a.trigger=false;
        }
        a.y+=dt*5.0e-5f;
    }
    auto& timeout=auxiliaryEffects_[5];
    if(timeout.active())timeout.y+=dt*2.5e-5f;
}

void Game::updateLegacyEffects(int dtMs){
    updateAuxiliaryEffects(dtMs);
    const float dt=float(dtMs);
    if(enemySpeedFactor_<1.0f)enemySpeedFactor_=std::min(1.0f,enemySpeedFactor_+dt*0.0001f);

    if(playerMaxSpeed_>0.03f)playerMaxSpeed_=std::max(0.03f,playerMaxSpeed_-dt*0.0000016f);
    else if(playerMaxSpeed_<0.03f)playerMaxSpeed_=std::min(0.03f,playerMaxSpeed_+dt*0.0000018f);
    player_.setMaxSpeed(playerMaxSpeed_);

    // 0x418766: global render brightness returns to 1 using the recovery
    // value set by the pickup dispatcher (normally 0.00018 per ms).
    if(brightnessScale_<1.0f)brightnessScale_=std::min(1.0f,brightnessScale_+dt*brightnessRecoveryRate_);

    // 0x418672..0x4186DD + camera call at 0x41A1F9: effect 6 is the
    // close-up camera displacement. It rises to 0.042, then returns slowly.
    // Original camera Y = baseY - 1.1*zoom, Z = baseZ + zoom.
    if(legacyCameraZoomRate_!=0.0f){
        legacyCameraZoom_+=dt*legacyCameraZoomRate_;
        if(legacyCameraZoom_>=0.042f && legacyCameraZoomRate_>0.f)
            legacyCameraZoomRate_=-0.000002f;
        if(legacyCameraZoom_<0.f){legacyCameraZoom_=0.f;legacyCameraZoomRate_=0.f;}
    }

    // 0x4187AB..0x418836: effect 8 is a decaying sinusoidal camera-angle
    // offset. The original truncates the result toward zero and adds it
    // to the base 0..2047 camera-angle unit.
    if(legacyOscAmplitude_>0.0f){
        legacyOscAmplitude_=std::max(0.0f,legacyOscAmplitude_-dt*0.01f);
        legacyOscPhase_+=dt*0.002f;
        while(legacyOscPhase_>=kTwoPi)legacyOscPhase_-=kTwoPi;
        legacyCameraYawOffset_=static_cast<int>(std::sin(legacyOscPhase_)*legacyOscAmplitude_);
    }else legacyCameraYawOffset_=0;
}

void Game::applyPickupEffect(PickupEffect effect){
    // 0x415726 applies a common visual kick for dispatcher ids >=5: global
    // brightness becomes 0.6 and recovers at 0.00018/ms. Individual branches
    // then add camera zoom (6), full blackout (7), or oscillation (8).
    const int id=static_cast<int>(effect);
    if(id>=5){brightnessScale_=0.6f;brightnessRecoveryRate_=0.00018f;}

    switch(effect){
        case PickupEffect::Score1000: score_+=1000;break;
        case PickupEffect::Time15000: timer_+=15000;break;
        case PickupEffect::ExtraLife: ++lives_;break;
        case PickupEffect::SlowEnemies: enemySpeedFactor_=0.3f;break;
        case PickupEffect::PlayerFast: playerMaxSpeed_=0.043f;player_.setMaxSpeed(playerMaxSpeed_);break;
        case PickupEffect::PlayerSlow: playerMaxSpeed_=0.012f;player_.setMaxSpeed(playerMaxSpeed_);break;
        case PickupEffect::CameraZoomIn: legacyCameraZoomRate_=0.0001f;break;
        case PickupEffect::Blackout: brightnessScale_=0.0f;break;
        case PickupEffect::CameraShake: legacyOscAmplitude_=120.0f;legacyOscPhase_=0.0f;legacyCameraYawOffset_=0;break;
    }
}

int Game::updatePickupRuntime(int dtMs,const PickupCollectorState& collector,bool allowPlayerCollection){
    for(PickupEffect e:pickups_.updateWithCollector(dtMs,field_,collector,entities_,rng_,timer_,lives_,allowPlayerCollection))
        applyPickupEffect(e);

    for(const auto& ev:pickups_.consumeAudioEvents()){
        const auto tag=pickupVoiceTag(ev.slot);
        if(ev.kind==PickupAudioEventKind::SpatialStart)queueDeathSpatialStart(tag,ev.logicalId,ev.x,ev.y,ev.z,ev.scalar);
        else if(ev.kind==PickupAudioEventKind::SpatialUpdate)queueDeathSpatialUpdate(tag,ev.x,ev.y,ev.z);
        else if(ev.kind==PickupAudioEventKind::SpatialStop)queueDeathSpatialStop(tag);
        else {
            if(ev.logicalId>=0x0Au && ev.logicalId<=0x0Du)spawnAuxiliaryEffect(int(ev.logicalId-0x0Au),ev.x,ev.y,ev.z);
            else if(ev.logicalId==0x31u)spawnAuxiliaryEffect(4,ev.x,ev.y,ev.z);
            queueDeathSpatialPlay(ev.logicalId,ev.x,ev.y,ev.z,ev.scalar);
        }
    }

    auto smash=pickups_.consumeSmashEvents();
    const int smashCount=static_cast<int>(smash.size());
    if(smashCount){
        effectEvents_.pickupSmashEvents+=smashCount;
        effectEvents_.pickupSmashDebrisParticles+=128*smashCount;
        effectEvents_.pickupSmashSfx0EEvents+=smashCount;
        pickupSmashEvents_.insert(pickupSmashEvents_.end(),smash.begin(),smash.end());
    }
    return smashCount;
}

void Game::advanceLegacyBackgroundScroll(int dtMs){
    legacyBackgroundVPhase_=LegacyBackgroundScroll::advance(legacyBackgroundVPhase_,dtMs);
}

void Game::spawnDeathBurst(float worldX,float worldY,float worldZ){
    // 0x417390: exactly 512 records. Each particle consumes three calls from
    // the global MSVC-compatible rand stream. Lifetime is global (0x9C4 ms).
    for(auto& p:deathBurstParticles_){
        const auto v=LegacyParticles::deathVelocity(rng_);
        p={worldX,worldY,worldZ,v.x,v.y,v.z};
    }
    deathBurstRemainingMs_=LegacyParticles::DeathLifetimeMs;
}

void Game::startDeathOverlay(float x,float y,float z,float r,float g,float b){
    // r340 DIRECT EXE 0x41BDB0: four-vertex additive impact quad.
    deathOverlay_={};
    deathOverlay_.active=true;
    deathOverlay_.x=x; deathOverlay_.y=y; deathOverlay_.z=z;
    deathOverlay_.r=r; deathOverlay_.g=g; deathOverlay_.b=b;
}

void Game::updateDeathOverlay(int dtMs){
    if(!deathOverlay_.active || dtMs<=0)return;
    const float dt=float(dtMs);
    // 0x41BEE0 tests the +Z corner before expansion. The first frame starts
    // exactly at .01 and therefore expands without fading; later frames fade.
    if(deathOverlay_.halfExtent>0.010000000707805157f){
        deathOverlay_.intensity-=dt*0.003000000026077032f;
        if(deathOverlay_.intensity<0.f){deathOverlay_.active=false;return;}
    }
    deathOverlay_.halfExtent+=dt*0.0001500000071246177f;
}

void Game::updateDeathBurst(int dtMs){
    if(dtMs<=0 || deathBurstRemainingMs_<=0)return;
    deathBurstRemainingMs_=std::max(0,deathBurstRemainingMs_-dtMs);
    const float dt=float(dtMs);
    // 0x417470 uses old vy for the Y Euler step, then stores vy-g*dt.
    for(auto& p:deathBurstParticles_){
        const float oldVy=p.vy;
        p.x+=p.vx*dt; p.y+=oldVy*dt; p.z+=p.vz*dt;
        p.vy=oldVy-LegacyParticles::DeathGravityPerMs*dt;
    }
}

void Game::initDeathTriColor(float worldX,float worldY,float worldZ){
    // 0x418200: one contiguous 480-record array, three MSVC rand calls per
    // record, all records start at the current death world position.
    for(auto& p:deathTriColorParticles_){
        const auto v=LegacyParticles::deathTriColorVelocity(rng_);
        p={worldX,worldY,worldZ,v.x,v.y,v.z};
    }
    deathTriColorInitialized_=true;
}

void Game::updateDeathTriColor(int dtMs){
    if(dtMs<=0 || !deathTriColorInitialized_)return;
    // Fresh r92 disassembly of 0x418290: x/z use velocity*dt; Y uses the old
    // vy for this Euler step, then vy is reduced by 1.5e-7*dt. No lifetime or
    // per-particle clipping is performed in this routine.
    for(auto& p:deathTriColorParticles_)
        LegacyParticles::updateDeathTriColor(p.x,p.y,p.z,p.vy,p.vx,p.vz,dtMs);
}


void Game::queueDeathSpatialPlay(std::size_t logicalId,float x,float y,float z,float scalar){
    deathAudioEvents_.push_back({DeathAudioEventKind::SpatialPlay,DeathAudioVoiceTag::None,logicalId,x,y,z,scalar,0.f});
}

void Game::resolveSpecialPreEntityContacts(){
    // DIRECT EXE 0x41612E..0x41623A is part of the common airborne updater and
    // therefore precedes pair contacts/movement in every live-world state.
    const auto& e=specialObjects_.eraser();
    if(e.active && e.speed!=0.f && entities_.resolveEraserAirContacts(e.worldX,e.worldZ))
        specialObjects_.requestEraserRedirect();
}

void Game::consumeEntityCollisionAudioEvents(){
    for(const auto& ev:entities_.consumeCollisionSfxEvents())
        queueDeathSpatialPlay(ev.logicalId,ev.x,ev.y,ev.z,ev.scalar);
}

void Game::consumeSpecialPostUpdateEvents(){
    // 0x415621..0x415650: homing pulse is positional logical SFX 0x22 with
    // scalar .1 at the just-updated homing XYZ.
    const int homingPulses=specialObjects_.consumeHomingSfx22Events();
    if(homingPulses>0){
        const auto& h=specialObjects_.homing();
        for(int i=0;i<homingPulses;++i)queueDeathSpatialPlay(0x22u,h.worldX,h.height,h.worldZ,0.1f);
    }
    const int n=specialObjects_.consumeEraserImpactEvents();
    effectEvents_.eraserImpactEvents+=n;
    effectEvents_.eraserDebrisParticles+=16*n;
    effectEvents_.eraserSfx23Events+=n;
}
void Game::queueDeathSpatialStart(DeathAudioVoiceTag voice,std::size_t logicalId,float x,float y,float z,float scalar){
    deathAudioEvents_.push_back({DeathAudioEventKind::SpatialStart,voice,logicalId,x,y,z,scalar,0.f});
}
void Game::queueDeathSpatialUpdate(DeathAudioVoiceTag voice,float x,float y,float z){
    deathAudioEvents_.push_back({DeathAudioEventKind::SpatialUpdate,voice,0u,x,y,z,1.f,0.f});
}
void Game::queueDeathSpatialStop(DeathAudioVoiceTag voice){
    deathAudioEvents_.push_back({DeathAudioEventKind::SpatialStop,voice,0u,0.f,0.f,0.f,1.f,0.f});
}
void Game::consumeCaptureStartedAudio(){
    // r253 DIRECT EXE 0x41855D..0x4185D0. The marker cell count is tested
    // after all preserved components have been flood-cleared. Only a non-empty
    // new capture plays simple logical SFX 6 (up01); an empty marker is silent.
    const std::uint8_t marker=field_.consumeStartedCaptureMarker();
    if(marker!=0 && field_.markerCellCount(marker)>0)queueDeathSimple(0x06u);
}

void Game::queueDeathSimple(std::size_t logicalId){
    deathAudioEvents_.push_back({DeathAudioEventKind::SimplePlay,DeathAudioVoiceTag::None,logicalId,0.f,0.f,0.f,1.f,0.f});
}
void Game::queueDeathMusicFade(float perMs){
    deathAudioEvents_.push_back({DeathAudioEventKind::MusicFadeOut,DeathAudioVoiceTag::None,0u,0.f,0.f,0.f,1.f,perMs});
}
void Game::queueMusicRequest(std::size_t trackId,float transitionScalar){
    deathAudioEvents_.push_back({DeathAudioEventKind::MusicRequest,DeathAudioVoiceTag::None,trackId,0.f,0.f,0.f,1.f,transitionScalar});
}

void Game::handleDeath(){
    if(lowTimeWarningActive_){ queueDeathSpatialStop(DeathAudioVoiceTag::LowTimeWarning); lowTimeWarningActive_=false; }
    --lives_;
    timer_=std::max(timer_,60<<9);
    // 0x41C030 clears the unfinished cut before entering its asynchronous
    // death/effect loop. This part is confirmed by the previous binary trace.
    for(const auto& t:player_.trail())
        if(field_.inside(t.x,t.y) && field_.at(t.x,t.y)==Field::Trail)
            field_.set(t.x,t.y,Field::Empty);
    player_.clearTrailForDeath();
    // Do not jump directly to GameOver when the last life is consumed. Fresh
    // r92 CFG around 0x41C540 proves 0x41C030 keeps running with lives==0 and
    // selects a distinct final-second branch before normal cleanup.

    // 0x41C030 entry writes the global diffuse/brightness multiplier
    // 0x53B684 to exactly 0.8 before the asynchronous death presentation.
    // Preserve the entry state even if a pickup effect had left a different
    // brightness immediately before death.
    brightnessScale_=0.8f;

    phase_=GamePhase::Dying;
    deathScene_={};
    deathScene_.durationMs=kDeathPresentationMs;
    deathScene_.worldX=player_.worldX();
    deathScene_.worldY=player_.visualY();
    deathScene_.worldZ=player_.worldZ();
    // 0x41C079 initializes the vertical death velocity to 0.00018. X/Z drift
    // comes from the live movement direction (normally +/-3e-5) unless a
    // direct-body lethal consumer supplied the exact crawler/homing impulse.
    deathScene_.velocityY=0.00018f;
    deathScene_.velocityX=pendingDeathDriftOverride_?pendingDeathDriftX_:player_.legacyDeathDriftX();
    deathScene_.velocityZ=pendingDeathDriftOverride_?pendingDeathDriftZ_:player_.legacyDeathDriftZ();
    // 0x41C04B..0x41C0AE: two positional sounds are started immediately.
    // ID 5 (fir1) is fire-and-forget; ID 0x11 (fir2) returns a voice handle
    // that is explicitly stopped when the tri-colour burst begins.
    deathAudioEvents_.clear();
    queueDeathSpatialPlay(0x05u,deathScene_.worldX,deathScene_.worldY,deathScene_.worldZ,1.0f);
    queueDeathSpatialStart(DeathAudioVoiceTag::InitialDeathVoice,0x11u,
                           deathScene_.worldX,deathScene_.worldY,deathScene_.worldZ,1.0f);
    // 0x41C4CB / 0x41C8C5..0x41C900: Dying owns the Xonix rotor state.
    // Radius is forced to .0057 and the persistent phase keeps advancing.
    deathScene_.rotorPhase=playerRotorPhase_;
    deathScene_.rotorRadius=0.00570000009611249f;
    pendingDeathDriftX_=pendingDeathDriftZ_=0.f;
    pendingDeathDriftOverride_=false;
    deathTriColorInitialized_=false;
    paused_=false;
}

void Game::updateDeathSequence(int dtMs){
    if(dtMs<=0)return;
    deathScene_.elapsedMs+=dtMs;
    updateDeathBurst(dtMs);
    updateDeathOverlay(dtMs);

    const float dt=float(dtMs);
    const int remaining=deathScene_.durationMs-deathScene_.elapsedMs;

    // 0x41C1B9..0x41C28F: exactly once per death, after the countdown becomes
    // strictly less than 0xF0A (3850 ms), optionally play one spoken cue.
    // 0x025B7898 gates the whole block. When disabled, the persistent selector
    // 0x0257F548 is NOT advanced. When enabled it advances first, then selects
    // out!/aaaa/ohoh/oyoy as indices 0/1/2/3. All four use spatial 0x40AE90
    // at the current Xonix XYZ with scalar 1.5.
    if(!deathScene_.earlySpeechCueConsumed && remaining<3850){
        deathScene_.earlySpeechCueConsumed=true;
        if(speechEnabled_){
            deathSpeechCycleIndex_=std::uint8_t((deathSpeechCycleIndex_+1u)&3u);
            static constexpr std::size_t kDeathSpeechIds[4]={0x27u,0x2Bu,0x2Cu,0x34u};
            queueDeathSpatialPlay(kDeathSpeechIds[deathSpeechCycleIndex_],
                                  deathScene_.worldX,deathScene_.worldY,deathScene_.worldZ,1.5f);
        }
    }

    // 0x41C8BC..0x41C900: death rotor phase is persistent and advances at
    // exactly .007 rad/ms while the normal gameplay rotor update is suspended.
    deathScene_.rotorPhase+=dt*0.007000000216066837f;
    while(deathScene_.rotorPhase>=kTwoPi) deathScene_.rotorPhase-=kTwoPi;
    deathScene_.rotorRadius=0.00570000009611249f;

    // Fresh direct disassembly of 0x41C2B7..0x41C4BF. Before 0x418200 is
    // triggered the dead Xonix is a small ballistic object. X/Z preserve the
    // direction/impact drift, Y starts with +0.00018 and gravity is 3.5e-7/ms.
    if(!deathScene_.triColorBurstStarted){
        deathScene_.worldY+=deathScene_.velocityY*dt; // old vy is used this step
        deathScene_.worldX+=deathScene_.velocityX*dt;
        deathScene_.worldZ+=deathScene_.velocityZ*dt;
        deathScene_.worldX=std::clamp(deathScene_.worldX,0.39f,0.61f);
        deathScene_.worldZ=std::clamp(deathScene_.worldZ,0.39f,0.61f);
        deathScene_.velocityY-=3.4999999343199306e-7f*dt;

        // 0x41C39C..0x41C4BB: crossing below Y=.008 creates the 480-record
        // tri-colour burst at the current position, teleports the presentation
        // Xonix to Y=.5, changes global brightness to .55 and replaces X/Z
        // velocity with a weak drift toward the legacy centre target.
        if(deathScene_.worldY<0.00800000037997961f){
            // 0x41C3D0 -> 0x41BDB0: cyan impact overlay overwrites any
            // earlier crawler-orange overlay at Xonix X/Z, fixed Y=.025.
            startDeathOverlay(deathScene_.worldX,0.02500000037252903f,deathScene_.worldZ,0.f,255.f,255.f);
            initDeathTriColor(deathScene_.worldX,deathScene_.worldY,deathScene_.worldZ);
            // 0x41C3DF..0x41C406: stop the retained fir2 voice, then play
            // spatial logical SFX 4 (fire) at scalar 1.3 from impact XYZ.
            if(!deathScene_.initialDeathVoiceStopped){
                queueDeathSpatialStop(DeathAudioVoiceTag::InitialDeathVoice);
                deathScene_.initialDeathVoiceStopped=true;
            }
            queueDeathSpatialPlay(0x04u,deathScene_.worldX,deathScene_.worldY,deathScene_.worldZ,1.2999999523162842f);
            deathScene_.triColorBurstStarted=true;
            deathScene_.worldY=0.5f;
            brightnessScale_=0.55f;
            // 0x41C43B starts the death-only camera shake at amplitude 13.
            deathScene_.cameraShakeAmplitude=13.0f;
            deathScene_.cameraShakePhase=0.0f;
            deathScene_.velocityX=(0.5015625357627869f-deathScene_.worldX)*0.000699999975040555f;
            deathScene_.velocityZ=(0.40156251192092896f-deathScene_.worldZ)*0.000699999975040555f;
        }
    }else if(remaining>=1000){
        // 0x41C715..0x41C79F: after the burst, but before the final second,
        // X/Z continue with the one-shot centre-directed velocity. The EXE
        // tests the signed target delta after integration and snaps to the
        // exact target when the step would cross it.
        constexpr float targetX=0.5015625357627869f;
        constexpr float targetZ=0.40156251192092896f;
        // r194 DIRECT EXE 0x41C751..0x41C795: the snap test is
        // (target-pos)*velocity < 0 (x87 C0 after fcomp 0), evaluated EVERY
        // frame with the one-shot velocity at [esp+4C]/[esp+50], which is never
        // cleared. r92..r193 compared against the pre-step delta: after the
        // first snap that delta became zero, the guard disabled further snaps
        // and the constant velocity carried Xonix past the start cell (e.g.
        // x=.4817,z=.3907 instead of .5015625/.4015625). The landing then ended
        // off target and jumped when gameplay resumed at logical (32,0).
        deathScene_.worldX+=deathScene_.velocityX*dt;
        deathScene_.worldZ+=deathScene_.velocityZ*dt;
        if((targetX-deathScene_.worldX)*deathScene_.velocityX<0.f) deathScene_.worldX=targetX;
        if((targetZ-deathScene_.worldZ)*deathScene_.velocityZ<0.f) deathScene_.worldZ=targetZ;
    }

    // 0x41C4BF..0x41C511: death-only camera oscillation. The same frame that
    // starts the tri-colour burst already consumes dt: amplitude -= .01*dt,
    // phase += .01*dt, and the shared x87 truncation helper stores the sinusoid as camera yaw offset.
    if(deathScene_.triColorBurstStarted){
        deathScene_.cameraShakeAmplitude=std::max(0.0f,deathScene_.cameraShakeAmplitude-dt*0.01f);
        deathScene_.cameraShakePhase+=dt*0.01f;
        while(deathScene_.cameraShakePhase>=kTwoPi)deathScene_.cameraShakePhase-=kTwoPi;
        deathScene_.cameraYawOffset=static_cast<int>(
            std::sin(deathScene_.cameraShakePhase)*deathScene_.cameraShakeAmplitude);
    }

    // 0x418290 runs later in the same frame as 0x418200, so update after the
    // phase-trigger check rather than before it.
    updateDeathTriColor(dtMs);

    // 0x41C540 splits the final second by remaining lives. With lives left,
    // the presentation Xonix is placed at Y=.1 once and descends at 9e-5/ms
    // back to the exact field height .008. With zero lives, a separate camera
    // phase advances instead; preserve those exact state variables here.
    if(remaining<1000){
        if(lives_>0){
            // DIRECT EXE 0x41C599..0x41C5BA writes logical player current/prev
            // coordinates to (32,0) at final-second entry, not at the end.
            if(!deathScene_.logicalRespawnPrepared){
                player_.prepareRespawnCoordinates();
                deathScene_.logicalRespawnPrepared=true;
            }
            if(!deathScene_.finalSecondRespawnStarted){
                deathScene_.worldY=0.1f;
                deathScene_.finalSecondRespawnStarted=true;
                // 0x41C569..0x41C594: retained spatial ID 7 / vint voice.
                queueDeathSpatialStart(DeathAudioVoiceTag::RespawnVoice,0x07u,
                                       deathScene_.worldX,deathScene_.worldY,deathScene_.worldZ,1.0f);
            }
            // 0x41C61B: normal respawn branch retracts rotor radius to .0047.
            deathScene_.rotorRadius=0.004699999932199717f;
            if(deathScene_.worldY>0.00800000037997961f){
                deathScene_.worldY=std::max(0.00800000037997961f,deathScene_.worldY-dt*0.00009000000136438757f);
                queueDeathSpatialUpdate(DeathAudioVoiceTag::RespawnVoice,
                                        deathScene_.worldX,deathScene_.worldY,deathScene_.worldZ);
            }
        }else{
            if(!deathScene_.zeroLivesAudioStarted){
                // 0x41C638..0x41C64C: simple haha + music fade-out, once.
                queueDeathSimple(0x18u);
                queueDeathMusicFade(0.0003000000142492354f);
                deathScene_.zeroLivesAudioStarted=true;
            }
            deathScene_.zeroLivesPhase+=dt*0.003000000026077032f;
            while(deathScene_.zeroLivesPhase>6.2831854820251465f)
                deathScene_.zeroLivesPhase-=6.2831854820251465f;
            deathScene_.zeroLivesPitch=std::min(-0.029999999329447746f,
                deathScene_.zeroLivesPitch+dt*0.0003000000142492354f);
            // 0x41C6A4..0x41C6B8: this is a render grayscale driver, not a
            // camera pitch. 165 - local*300 evolves from 255 toward 174.
            deathScene_.zeroLivesGray=165.0f-deathScene_.zeroLivesPitch*300.0f;
        }
    }

    // DIRECT EXE 0x41C816: 0x4185F0 runs before the live death-world
    // entity/pickup updates. This matters for Slow recovery because the
    // recovered enemy factor is consumed by 0x416110/0x416950 in this same frame.
    updateLegacyEffects(dtMs);

    // Confirmed semantic behaviour of 0x41C030: the death sequence is not a
    // frozen pause. Capture animation and major world systems continue.
    field_.updateCaptureAnimations(dtMs);
    resolveSpecialPreEntityContacts();
    entities_.update(dtMs,field_,rng_,enemySpeedFactor_,deathScene_.triColorBurstStarted);
    consumeEntityCollisionAudioEvents();
    effectEvents_.airErosionImpactEvents+=entities_.consumeAirErosionImpactEvents();
    effectEvents_.airErosionDebrisHelperCalls+=entities_.consumeAirErosionDebrisHelperCalls();
    specialObjects_.update(dtMs,field_,player_,enemySpeedFactor_,rng_);
    consumeSpecialPostUpdateEvents();
    updatePickupRuntime(dtMs,{deathScene_.worldX,deathScene_.worldY,deathScene_.worldZ});
    advanceLegacyBackgroundScroll(dtMs);
    // 0x41C030 owns 0x53B684 while the death routine is active. Gameplay
    // pickup-recovery must not brighten .8/.55 back toward 1 during this loop.
    brightnessScale_=deathScene_.triColorBurstStarted?0.55f:0.8f;
    updateScoreAndPercent();

    // 0x41C625 branches only when the signed remaining countdown is < 0.
    // remaining == 0 therefore still renders one final Dying frame.
    if(deathScene_.elapsedMs>deathScene_.durationMs){
        playerRotorPhase_=deathScene_.rotorPhase;
        if(lives_<=0){
            // 0x41C6BE does NOT jump to 0x41CEA0. It initializes the local
            // event queue (0x4093E0), keeps the same death-world presentation
            // alive, and polls 0x409410 until an event arrives or the negative
            // countdown crosses -60000 at 0x41C6FE.
            beginGameOverTail();
        }else{
            // 0x41CE2C..0x41CE64: normal respawn completion first plays
            // positional ID 9 / efly at the final Xonix XYZ, scalar 1.0, then
            // explicitly stops the retained ID 7 / vint voice.
            queueDeathSpatialPlay(0x09u,deathScene_.worldX,deathScene_.worldY,deathScene_.worldZ,1.0f);
            queueDeathSpatialStop(DeathAudioVoiceTag::RespawnVoice);
            // 0x41CE67 -> 0x416890: reset/reseed all crawler records before
            // returning to the live gameplay loop.
            entities_.resetCrawlerTransition();
            // 0x41CEE1 -> 0x422EE0 -> 0x40B240: a successful respawn requests
            // a new least-used gameplay track. This was absent from r163.
            currentMusicTrack_=airxonix::LegacyMusicSelectorTrace::select(musicTrackUsage_,rng_.next());
            queueMusicRequest(currentMusicTrack_,0.0005000000237487257f);
            player_.finishRespawn();
            player_.setMaxSpeed(playerMaxSpeed_);
            deathScene_={};
            deathTriColorInitialized_=false;
            phase_=GamePhase::Gameplay;
        }
    }
}

void Game::beginGameOverTail(){
    phase_=GamePhase::GameOver;
    paused_=false;
    pauseLatch_=false;
    presentationArmed_=false;
    gameOverScene_={};
    gameOverScene_.timeoutMs=kGameOverTailMs;
    // DIRECT EXE 0x41C6BE..0x41C706: Game Over begins only after the
    // 4000-ms death countdown is strictly negative. The same signed countdown
    // remains live in this stack frame, so preserve the crossing-frame
    // overshoot instead of resetting the tail clock to zero.
    gameOverScene_.elapsedMs=std::max(0,deathScene_.elapsedMs-deathScene_.durationMs);
    gameOverScene_.visual=deathScene_;
    // 0x41C6C4..0x41C6E1: after initializing the event queue, the same speech
    // enable global 0x025B7898 gates simple SFX 0x1F / gove exactly once.
    if(speechEnabled_) queueDeathSimple(0x1Fu);
    // At entry the zero-lives local phase/scalar already belong to this same
    // 0x41C030 stack frame; preserve them rather than reinitializing.
}

void Game::updateGameOverTail(const InputState& input,int dtMs){
    if(input.quit){quit_=true;return;}
    if(dtMs<0)dtMs=0;

    // 0x4093E0 initializes an empty ring queue once. 0x409410 then consumes
    // an event; model that with release-arming so the key that caused the
    // preceding death cannot immediately dismiss the tail in the same frame.
    const bool any=input.action||input.back||input.pause||input.up||input.down||input.left||input.right;
    if(!gameOverScene_.inputArmed){
        if(!any)gameOverScene_.inputArmed=true;
    }else if(any){
        // 0x41C6F1 -> 0x41CE87: event-driven Game Over exit plays simple
        // logical SFX 0x16 / clc2 before returning. The timeout exit at
        // 0x41CE7C bypasses this sound.
        queueDeathSimple(0x16u);
        // r245 DIRECT EXE 0x419698 -> 0x41A8AF returns score 0x257DA20;
        // wrapper 0x424F46 then calls 0x40F160(score,mode). Only -1/abort
        // bypasses Records, so an ordinary Game Over must not jump to M1.
        enterPostGameRecords(static_cast<std::uint32_t>(std::max(0,score_)));
        return;
    }

    if(dtMs==0)return;
    gameOverScene_.elapsedMs+=dtMs;
    // 0x41C6FE uses signed JL against -60000: exactly -60000 remains in the
    // live Game Over world; only a value strictly below it exits. The timeout
    // check occurs before the common 0x41C7D9 world-update tail.
    if(gameOverScene_.elapsedMs>gameOverScene_.timeoutMs){
        enterPostGameRecords(static_cast<std::uint32_t>(std::max(0,score_)));
        return;
    }
    auto& d=gameOverScene_.visual;
    const float dt=float(dtMs);

    // The zero-lives branch at 0x41C632 continues every frame after the main
    // 4000-ms countdown becomes negative.
    d.zeroLivesPhase+=dt*0.003000000026077032f;
    while(d.zeroLivesPhase>kTwoPi)d.zeroLivesPhase-=kTwoPi;
    d.zeroLivesPitch=std::min(-0.029999999329447746f,
        d.zeroLivesPitch+dt*0.0003000000142492354f);
    d.zeroLivesGray=165.0f-d.zeroLivesPitch*300.0f;

    // Common 0x41C7D9+ death-world loop remains alive during this tail.
    d.rotorPhase+=dt*0.007000000216066837f;
    while(d.rotorPhase>=kTwoPi)d.rotorPhase-=kTwoPi;
    d.rotorRadius=0.00570000009611249f;
    updateDeathTriColor(dtMs);
    updateDeathBurst(dtMs);
    // 0x41C816 common death-world loop: 0x4185F0 precedes entities/pickups.
    updateLegacyEffects(dtMs);
    field_.updateCaptureAnimations(dtMs);
    resolveSpecialPreEntityContacts();
    entities_.update(dtMs,field_,rng_,enemySpeedFactor_);
    consumeEntityCollisionAudioEvents();
    effectEvents_.airErosionImpactEvents+=entities_.consumeAirErosionImpactEvents();
    effectEvents_.airErosionDebrisHelperCalls+=entities_.consumeAirErosionDebrisHelperCalls();
    specialObjects_.update(dtMs,field_,player_,enemySpeedFactor_,rng_);
    consumeSpecialPostUpdateEvents();
    updatePickupRuntime(dtMs,{d.worldX,d.worldY,d.worldZ});
    advanceLegacyBackgroundScroll(dtMs);
    brightnessScale_=d.triColorBurstStarted?0.55f:0.8f;
    updateScoreAndPercent();

}

void Game::beginInterLevel(std::size_t nextLevel){
    if(lowTimeWarningActive_){ queueDeathSpatialStop(DeathAudioVoiceTag::LowTimeWarning); lowTimeWarningActive_=false; }
    // r272 DIRECT EXE 0x41A98F..0x41A9F8: completion scoring runs
    // before the 4000-ms presentation. Integer time uses positive truncation;
    // float additions use the shared x87 truncate-toward-zero helper.
    score_+=std::max(0,timer_)/20;
    score_=legacyTruncAdd(score_,1000.0f*float(capturePercent_-100)*difficultyScale_);
    // DIRECT EXE 0x41A980: dedicated 4000-ms live-world transition.
    phase_=GamePhase::InterLevel;
    paused_=false;
    interLevelScene_={};
    interLevelScene_.nextLevel=nextLevel;
    interLevelScene_.durationMs=kInterLevelPresentationMs;
    interLevelScene_.xonixHeight=player_.visualY();
    interLevelScene_.cinematicYOffset=-0.10000000149011612f;
    // 0x41AA55..0x41AA63: the original consumes one shared MSVC rand() call,
    // masks it with 7, and uses that integer as the initial cinematic phase.
    interLevelScene_.cinematicPhase=float(rng_.mask(7));
    interLevelScene_.rotorPhase=playerRotorPhase_;

    // 0x41A9F8: simple ID 0x19 / "game". 0x41AA1C asks the music system for
    // a -0.001/ms fade. 0x41AA41 starts retained spatial ID 7 / "vint".
    queueDeathSimple(0x19u);
    queueDeathMusicFade(0.0010000000474974513f);
    queueDeathSpatialStart(DeathAudioVoiceTag::InterLevelVoice,0x07u,
                           player_.worldX(),interLevelScene_.xonixHeight,player_.worldZ(),1.f);
}

void Game::updateLevelIntro(int dtMs){
    if(dtMs<=0 || !levelIntroScene_.active())return;
    // r291: music selection is deliberately deferred until this intro returns.

    auto finishIntro=[this](){
        selectLevelMusic();
        levelEntryScene_={};
        levelEntryScene_.running=true;
        brightnessScale_=0.f;
        queueDeathSpatialStart(DeathAudioVoiceTag::LevelEntryVoice,0x07u,
                               levelEntryScene_.worldX,levelEntryScene_.worldY,levelEntryScene_.worldZ,1.f);
        // r296 DIRECT EXE 0x41CF84..0x41CFC7: when speech is enabled,
        // decrement-and-mask the persistent selector and play spatial ID 41/42.
        if(speechEnabled_){
            levelEntrySpeechSelector_=(levelEntrySpeechSelector_-1)&1;
            queueDeathSpatialPlay(0x29u+std::size_t(levelEntrySpeechSelector_),
                                  levelEntryScene_.worldX,levelEntryScene_.worldY,levelEntryScene_.worldZ,
                                  0.69999998807907104f);
        }
    };

    // r347 DIRECT EXE 0x41D601..0x41D682: the capable path presents the
    // previous light value, then integrates light += dt*.135. At 255 it clamps
    // and reverses direction; only a subsequent crossing below zero exits the
    // whole plaque routine. EBP=3000 controls the motion, not routine lifetime.
    levelIntroScene_.presentedLightByte=levelIntroScene_.lightByte;
    levelIntroScene_.lightByte+=float(dtMs)*levelIntroScene_.lightVelocityPerMs;
    if(levelIntroScene_.lightByte>=255.f){
        levelIntroScene_.lightByte=255.f;
        levelIntroScene_.lightVelocityPerMs=-levelIntroScene_.lightVelocityPerMs;
    }else if(levelIntroScene_.lightByte<0.f){
        levelIntroScene_.finished=true;
        finishIntro();
        return;
    }

    // r286 DIRECT EXE 0x41D696..0x41D79C: countdown is decremented before
    // choreography is selected. The current dt belongs to the branch selected
    // by the post-decrement remaining time, even on a threshold crossing frame.
    levelIntroScene_.elapsedMs+=dtMs;
    const int remaining=levelIntroScene_.remainingMs();
    // r288 DIRECT EXE 0x41D688..0x41D6B8: Xonix presentation Y descends by
    // dt*6e-5 and the intro camera follows exactly .103 above it. X/Z remain
    // fixed for the whole startup presentation.
    levelIntroScene_.worldY-=float(dtMs)*0.00005999999848427251f;
    levelIntroScene_.cameraY=levelIntroScene_.worldY+0.10300000011920929f;
    // r289 DIRECT EXE 0x41D7BA..0x41D849: intro keeps the Xonix
    // propeller and four-node orbit alive. 0x420BB0 receives min(dt*.04,pi/2),
    // DA94 advances by dt*.005, and with no active trail DA98 is .0045.
    playerPropellerStep_=std::min(float(dtMs)*0.03999999910593033f,kPi*0.5f);
    playerPropellerPhase_-=playerPropellerStep_;
    while(playerPropellerPhase_<0.f)playerPropellerPhase_+=kTwoPi;
    playerRotorPhase_+=float(dtMs)*0.004999999888241291f;
    while(playerRotorPhase_>=kTwoPi)playerRotorPhase_-=kTwoPi;
    playerRotorRadius_=0.0044999998062849045f;
    if(!levelIntroScene_.cue14Consumed && remaining<2500){
        deathAudioEvents_.push_back({DeathAudioEventKind::SimplePlay,DeathAudioVoiceTag::None,0x14u});
        levelIntroScene_.cue14Consumed=true;
    }
    if(remaining>1000){
        levelIntroScene_.plaqueY=std::min(-0.02500000037252903f,
            levelIntroScene_.plaqueY+float(dtMs)*0.00009999999747378752f);
    }else{
        if(!levelIntroScene_.cue5Consumed){
            deathAudioEvents_.push_back({DeathAudioEventKind::SimplePlay,DeathAudioVoiceTag::None,0x05u});
            levelIntroScene_.cue5Consumed=true;
        }
        levelIntroScene_.plaqueY-=float(dtMs)*0.00005999999848427251f;
        // r344 DIRECT EXE 0x41D736..0x41D77A. The LEV2/"Этап" master is
        // rotated around Z only in the final second: angle += dt*.01, wrapping
        // at 2*pi. 0x41D5CC initializes the angle local to zero;
        // 0x41D5E0 is the independent plaque-Y local (-0.1), not the angle.
        levelIntroScene_.plaqueAngleRad+=float(dtMs)*0.009999999776482582f;
        while(levelIntroScene_.plaqueAngleRad>=kTwoPi)levelIntroScene_.plaqueAngleRad-=kTwoPi;
        levelIntroScene_.digitLateX+=float(dtMs)*1.9999999494757503e-5f;
    }

    // r284 DIRECT EXE 0x41D7A0..0x41D7AD: the startup presentation advances
    // airborne enemies (0x416110), shared pickups (0x4179B0) and cyclic world
    // background (0x41FD50), but does not enter the player/timer/crawler loop.
    resolveSpecialPreEntityContacts();
    entities_.updateAirOnly(dtMs,field_,rng_,enemySpeedFactor_);
    consumeEntityCollisionAudioEvents();
    effectEvents_.airErosionImpactEvents+=entities_.consumeAirErosionImpactEvents();
    effectEvents_.airErosionDebrisHelperCalls+=entities_.consumeAirErosionDebrisHelperCalls();
    updatePickupRuntime(dtMs,{levelIntroScene_.worldX,levelIntroScene_.worldY,levelIntroScene_.worldZ},true);
    advanceLegacyBackgroundScroll(dtMs);
    // r347: unlike the old 3000-ms approximation, routine cleanup is owned by
    // the descending-light crossing at the top of a later capable-path frame.
}

void Game::updateLevelEntry(int dtMs){
    if(dtMs<=0 || !levelEntryScene_.active())return;
    auto& s=levelEntryScene_;

    // r295 DIRECT EXE 0x41CFDC..0x41D056: the local light value starts at
    // zero and advances by dt*0.17241378.  The original tests for 255 at the
    // top of the next loop, so retain one fully-lit frame before cleanup.
    if(s.light>=255.f){
        s.running=false;
        brightnessScale_=1.f;
        queueDeathSpatialStop(DeathAudioVoiceTag::LevelEntryVoice);
        queueDeathSpatialPlay(0x09u,s.worldX,s.worldY,s.worldZ,1.f);
        return;
    }

    const float dt=float(dtMs);
    s.light=std::min(255.f,s.light+dt*0.17241378128528595f);
    brightnessScale_=s.light*(1.f/255.f);
    s.cameraAngle2=static_cast<int>(-302.f-(255.f-s.light)*0.1666666716337204f);

    // 0x41D071..0x41D0CC: falling Xonix with a decelerating descent rate.
    if(s.worldY>=0.00800000037997961f){
        s.worldY-=dt*s.descentPerMs;
        s.descentPerMs-=dt*1.0499999802959792e-7f;
        if(s.worldY<0.00800000037997961f)s.worldY=0.00800000037997961f;
    }

    // 0x41D0CC..0x41D13B: camera follows the presentation Xonix.
    s.cameraX=(s.worldX-0.40000000596046448f)*0.5f+0.44999998807907104f;
    s.cameraZ=(s.worldZ-0.40000000596046448f)*0.5f+0.34999999403953552f;
    s.cameraY=(s.worldY-0.00800000037997961f)*0.69999998807907104f+0.10300000011920929f;

    resolveSpecialPreEntityContacts();
    entities_.updateAirOnly(dtMs,field_,rng_,enemySpeedFactor_);
    consumeEntityCollisionAudioEvents();
    effectEvents_.airErosionImpactEvents+=entities_.consumeAirErosionImpactEvents();
    effectEvents_.airErosionDebrisHelperCalls+=entities_.consumeAirErosionDebrisHelperCalls();
    // r297 DIRECT EXE 0x41D402..0x41D409: the second startup scene also
    // advances homing 0x415490 and eraser 0x414F30 every frame.
    specialObjects_.update(dtMs,field_,player_,enemySpeedFactor_,rng_);
    consumeSpecialPostUpdateEvents();
    updatePickupRuntime(dtMs,{s.worldX,s.worldY,s.worldZ},true);
    advanceLegacyBackgroundScroll(dtMs);
    // 0x41D244 calls 0x4194D0 before world submission, so erosion during
    // the entrance is reflected in captured percent/score immediately.
    updateScoreAndPercent();

    playerPropellerStep_=std::min(dt*0.03999999910593033f,kPi*0.5f);
    playerPropellerPhase_-=playerPropellerStep_;
    while(playerPropellerPhase_<0.f)playerPropellerPhase_+=kTwoPi;
    playerRotorPhase_+=dt*0.004999999888241291f;
    while(playerRotorPhase_>=kTwoPi)playerRotorPhase_-=kTwoPi;
    playerRotorRadius_=0.0044999998062849045f;

    queueDeathSpatialUpdate(DeathAudioVoiceTag::LevelEntryVoice,s.worldX,s.worldY,s.worldZ);
}

void Game::updateInterLevel(int dtMs){
    if(dtMs<=0)return;
    interLevelScene_.elapsedMs+=dtMs;
    const int remaining=interLevelScene_.remainingMs();
    const float dt=float(dtMs);

    // 0x41ABDA..0x41AC02: Xonix rises during the transition and the retained
    // vint voice follows the moving source every frame.
    interLevelScene_.xonixHeight += dt*0.000022000000171829015f;
    queueDeathSpatialUpdate(DeathAudioVoiceTag::InterLevelVoice,
                            player_.worldX(),interLevelScene_.xonixHeight,player_.worldZ());

    // 0x41AA8D..0x41AB66: once only after remaining < 3000 ms.  The persistent
    // selector advances before dispatch; 0,1,2,3 map to cmex,yes1,cool,that.
    if(!interLevelScene_.speechCueConsumed && remaining<3000){
        interLevelScene_.speechCueConsumed=true;
        interLevelSpeechSelector_=(interLevelSpeechSelector_+1)&3;
        if(speechEnabled_){
            static constexpr std::size_t ids[4]={0x33u,0x2Du,0x2Eu,0x2Fu};
            queueDeathSpatialPlay(ids[interLevelSpeechSelector_],player_.worldX(),
                                  interLevelScene_.xonixHeight,player_.worldZ(),1.5f);
        }
    }

    // 0x41AB69 threshold is 0x09F6 = 2550 ms.  0x43FC68 is the master SFX
    // scalar (proved by 0x40AE50/0x40AE90/0x40A890), not 0x53B684 brightness.
    // Visual light RGB is remaining*0.1, equivalent to remaining/2550 of full
    // 255 RGB; SFX master is scaled by the same remaining/2550 ratio.
    if(remaining<2550){
        const float scale=std::max(0.f,float(remaining)/2550.f);
        interLevelScene_.lightScale=scale;
        interLevelScene_.sfxMasterScale=scale;
    }

    // 0x41ABB1..0x41ABD7: this is a world-Y translation, not a pitch angle.
    // It rises from -0.1 toward -0.038 at 8e-5 per millisecond.
    interLevelScene_.cinematicYOffset=std::min(-0.03799999877810478f,
        interLevelScene_.cinematicYOffset+dt*0.00007999999797903001f);
    // 0x41ADD6..0x41AE0B: the mirrored cinematic pair uses a shared phase
    // advanced by 0.002/ms. Rotations are cos(phase)*0.27 and
    // sin(phase*1.3)*0.21; the second mesh negates both rotations.
    interLevelScene_.cinematicPhase += dt*0.0020000000949949026f;
    while(interLevelScene_.cinematicPhase>=kTwoPi)interLevelScene_.cinematicPhase-=kTwoPi;

    // r343 DIRECT EXE 0x41AC97..0x41ACCF: the level-complete loop keeps
    // Xonix's central propeller alive. It feeds min(dt*.04,pi/2) to 0x420BB0
    // every frame, exactly like the startup/finale presentation. The previous
    // native transition advanced only the four outer rotor nodes.
    playerPropellerStep_=std::min(dt*0.03999999910593033f,kPi*0.5f);
    playerPropellerPhase_-=playerPropellerStep_;
    while(playerPropellerPhase_<0.f)playerPropellerPhase_+=kTwoPi;

    interLevelScene_.rotorPhase += dt*0.013000000268220901f;
    while(interLevelScene_.rotorPhase>=kTwoPi)interLevelScene_.rotorPhase-=kTwoPi;

    // Direct trace of 0x0041A980 disproves the old frozen-frame hypothesis.
    // During the 4000-ms sequence the x86 build continues its world systems:
    // 0x4185F0 effects, 0x416110 air enemies, 0x416950 crawlers,
    // 0x4179B0 pickups, 0x415490 homing and 0x414F30 eraser. Player movement
    // and the normal level timer are not advanced here.
    // 0x41AC71: 0x4185F0 runs before air/crawler/pickup updates.
    updateLegacyEffects(dtMs);
    field_.updateCaptureAnimations(dtMs);
    resolveSpecialPreEntityContacts();
    entities_.update(dtMs,field_,rng_,enemySpeedFactor_,false,remaining>2000);
    consumeEntityCollisionAudioEvents();
    effectEvents_.airErosionImpactEvents+=entities_.consumeAirErosionImpactEvents();
    effectEvents_.airErosionDebrisHelperCalls+=entities_.consumeAirErosionDebrisHelperCalls();
    specialObjects_.update(dtMs,field_,player_,enemySpeedFactor_,rng_);
    consumeSpecialPostUpdateEvents();
    // DIRECT EXE 0x41AC86 -> 0x4179B0: there is no cinematic suppression
    // flag. The shared routine compares against global Xonix X/Z and requires
    // Xonix Y < .03, so collection remains live only during the early rise.
    updatePickupRuntime(dtMs,{player_.worldX(),interLevelScene_.xonixHeight,player_.worldZ()});
    // 0x41AC8C -> 0x41FD50: prepared background V scroll is live here too.
    advanceLegacyBackgroundScroll(dtMs);
    updateScoreAndPercent();

    // 0x41B158 gates the separate 0x417700 debris updater/renderer with
    // remaining > 2000 ms. Entities::update receives the same literal gate.
    if(interLevelScene_.elapsedMs>=interLevelScene_.durationMs){
        const std::size_t next=interLevelScene_.nextLevel;
        // 0x41B1E2..0x41B1F6 stops retained vint before transition cleanup.
        queueDeathSpatialStop(DeathAudioVoiceTag::InterLevelVoice);
        // 0x41B1FC..0x41B220: stop and invalidate all six pickup retained handles.
        for(std::size_t i=0;i<6;++i)queueDeathSpatialStop(pickupVoiceTag(i));
        playerRotorPhase_=interLevelScene_.rotorPhase;
        loadLevel(mode_,next);
    }
}

void Game::beginFinalSequence(){
    if(lowTimeWarningActive_){ queueDeathSpatialStop(DeathAudioVoiceTag::LowTimeWarning); lowTimeWarningActive_=false; }
    // r272 DIRECT EXE 0x41B2A0 prologue. Preserve sequential truncation.
    score_+=std::max(0,timer_)/20;
    score_=legacyTruncAdd(score_,1000.0f*float(capturePercent_-100)*difficultyScale_);
    score_=legacyTruncAdd(score_,5000.0f*float(lives_)*difficultyScale_);
    score_=legacyTruncAdd(score_,5000.0f*difficultyScale_);
    phase_=GamePhase::FinalSequence;
    finalScene_={};
    finalScene_.xonixWorldX=player_.worldX();
    finalScene_.xonixWorldZ=player_.worldZ();
    finalScene_.presentationY=player_.visualY();
    finalScene_.xonixHeight=std::min(0.04f,finalScene_.presentationY);
    const float dx=finalScene_.xonixWorldX-.5f,dz=finalScene_.xonixWorldZ-.5f;
    finalScene_.orbitRadius=std::sqrt(dx*dx+dz*dz);
    if(finalScene_.orbitRadius<0.0001f)finalScene_.orbitRadius=0.0001f; // 0x41B3E0 guard
    finalScene_.orbitAngle=std::atan2(dz,dx);
    if(finalScene_.orbitAngle<0.f)finalScene_.orbitAngle+=kTwoPi;
    finalScene_.pickupSmashes=0;
    // DA94 is a persistent rotor phase global in the original; entering the
    // finale does not rebuild the Xonix model from phase zero.
    finalScene_.rotorPhase=playerRotorPhase_;
    finalScene_.rotorRadius=.0037f; // 0x41B6FC / 0x257DA98
    finalScene_.sceneLight255=255.0f;
    finalScene_.fadeScalar=1.0f;
    finalScene_.exitFadePerMs=0.0f;
    finalScene_.presentationAngle=-0.11999999731779099f;
    finalScene_.presentationAngleRate=1.9999999494757503e-5f;
    finalScene_.inputArmed=false;
    finalScene_.exitRequested=false;
    // r143 0x41B423..0x41B43B: finale audio entry contract.
    // comp one-shot, fade current music at -0.001/ms, then queue track 7
    // with transition scalar 0.0005 through 0x40B240.
    queueDeathSimple(0x17u);
    queueDeathMusicFade(0.0010000000474974513f);
    queueMusicRequest(7u,0.0005000000237487257f);
    // 0x41B468..0x41B478: one shared MSVC rand call seeds the mirrored
    // finale cinematic pair. This must consume the common RNG stream.
    finalScene_.cinematicPhase=float(rng_.mask(7));
    // 0x41B2C0 makes the homing object's motion factor negative during finale.
    brightnessScale_=1.0f;
}

void Game::updateFinalSequence(const InputState& input,int dtMs){
    if(dtMs<=0)return;
    const float dt=float(dtMs);
    finalScene_.elapsedMs+=dtMs;

    // DIRECT EXE r189 0x41B47C..0x41B4BD: one presentation angle starts at
    // -0.12, advances at +0.00002/ms and clamps at -0.03. Once exit starts
    // the rate is changed to -0.00003/ms and the clamp no longer catches it.
    finalScene_.presentationAngle += dt*finalScene_.presentationAngleRate;
    if(finalScene_.presentationAngleRate>0.f && finalScene_.presentationAngle>=-0.029999999329447746f)
        finalScene_.presentationAngle=-0.029999999329447746f;

    // 0x41B4BE..0x41B4E5: local presentation Y rises at 0.00002/ms. The
    // raw updated value is tested BEFORE the clamp. If it is <= .1 the loop
    // later calls 0x4185F0; once it overshoots .1, the EXE clamps to .1 and
    // permanently takes the post-rise branch instead.
    const float rawPresentationY=finalScene_.presentationY+dt*1.9999999494757503e-5f;
    const bool runLegacyEffectsThisFrame=rawPresentationY<=0.10000000149011612f;
    finalScene_.presentationY=std::min(0.10000000149011612f,rawPresentationY);
    finalScene_.xonixHeight=std::min(0.04f,finalScene_.presentationY);

    // 0x41B4EF..0x41B55F: ignore all queued input for seven seconds. On the
    // first frame after the gate the original clears the old event ring; only
    // a fresh subsequent event starts the exit. Auto-exit uses the same path.
    const bool anyEvent=input.action||input.back||input.pause||input.select||
        input.up||input.down||input.left||input.right||input.legacyPressedCode>=0;
    if(!finalScene_.inputArmed && finalScene_.elapsedMs>kFinalInputArmMs){
        finalScene_.inputArmed=true;
    }else if(!finalScene_.exitRequested &&
             ((finalScene_.inputArmed && anyEvent)||finalScene_.elapsedMs>kFinalAutoExitMs)){
        finalScene_.exitRequested=true;
        finalScene_.exitFadePerMs=0.10000000149011612f;
        finalScene_.presentationAngleRate=-2.9999999242136255e-5f;
    }

    // 0x41B560..0x41B5B5 is a pre-pass, not a replacement for 0x4179B0.
    // It shortens long timers and respawns already-grounded pickups; the smash
    // helper is gated by scene light > 100. Ordinary pickup motion/collection
    // still runs later through the common routine.
    (void)pickups_.prepareFinalSceneFrame(field_,rng_,timer_,finalScene_.sceneLight255);

    // 0x41B5C2: shared effect recovery is part of the finale only while the
    // local Xonix presentation Y has not overshot .1. The overshoot frame
    // itself skips this call after clamping, matching the original branch.
    if(runLegacyEffectsThisFrame) updateLegacyEffects(dtMs);

    // 0x41B645..0x41B72C updates the orbit BEFORE 0x4179B0. Calculate the
    // same-frame collector coordinates now; stored scene state is committed
    // below where r198 already updates the orbit.
    float collectorAngle=finalScene_.orbitAngle-dt*0.0004f;
    while(collectorAngle<0.f)collectorAngle+=kTwoPi;
    const float collectorX=.5f+std::cos(collectorAngle)*finalScene_.orbitRadius;
    const float collectorZ=.5f+std::sin(collectorAngle)*finalScene_.orbitRadius;

    // The final movie keeps the live game-world simulation running. 0x417700
    // is deliberately disabled: 0x41BCDD compares boolean inputArmed (0/1)
    // against 2000, so its branch is unreachable in the original.
    field_.updateCaptureAnimations(dtMs);
    resolveSpecialPreEntityContacts();
    entities_.update(dtMs,field_,rng_,enemySpeedFactor_,false,false);
    consumeEntityCollisionAudioEvents();
    effectEvents_.airErosionImpactEvents+=entities_.consumeAirErosionImpactEvents();
    effectEvents_.airErosionDebrisHelperCalls+=entities_.consumeAirErosionDebrisHelperCalls();
    specialObjects_.update(dtMs,field_,player_,enemySpeedFactor_,rng_,-1.0f);
    consumeSpecialPostUpdateEvents();
    const int smashEvents=updatePickupRuntime(dtMs,{collectorX,finalScene_.xonixHeight,collectorZ});
    finalScene_.pickupSmashes+=smashEvents;
    // 0x41B7EB -> 0x41FD50.
    advanceLegacyBackgroundScroll(dtMs);

    // 0x41B5CA..0x41B606: visual light starts at 255. While >=200 it loses
    // 0.05/ms. The second fade term is the compiler's stale stack slot before
    // exit; r189 deterministically models the intended pre-exit value as zero.
    // Once exit is requested the exact written value is 0.1/ms.
    if(finalScene_.sceneLight255>=200.0f)
        finalScene_.sceneLight255-=dt*0.05000000074505806f;
    finalScene_.sceneLight255-=dt*finalScene_.exitFadePerMs;
    finalScene_.fadeScalar=std::clamp(finalScene_.sceneLight255/255.0f,0.0f,1.0f);

    // 0x41B645..0x41B72C: phase -= dt*0.0004; x/z = 0.5 + cos/sin*radius.
    finalScene_.orbitAngle-=dt*0.0004f;
    while(finalScene_.orbitAngle<0.f)finalScene_.orbitAngle+=kTwoPi;
    finalScene_.xonixWorldX=.5f+std::cos(finalScene_.orbitAngle)*finalScene_.orbitRadius;
    finalScene_.xonixWorldZ=.5f+std::sin(finalScene_.orbitAngle)*finalScene_.orbitRadius;

    finalScene_.rotorRadius=.0037f;

    // r208 DIRECT EXE 0x41B7D8..0x41B833 -> 0x420BB0. The finale keeps
    // the central two-blade Xonix propeller alive independently from DA94.
    // It uses min(dt*0.04, pi/2) each frame, exactly like the accelerated
    // cutting branch, and subtracts that step from the prepared-mesh phase.
    playerPropellerStep_=std::min(dt*0.03999999910593033f,kPi*0.5f);
    playerPropellerPhase_-=playerPropellerStep_;
    while(playerPropellerPhase_<0.f)playerPropellerPhase_+=kTwoPi;

    finalScene_.cinematicPhase += dt*0.0020000000949949026f;
    while(finalScene_.cinematicPhase>=kTwoPi)finalScene_.cinematicPhase-=kTwoPi;

    // r188 correction, now applied to runtime: DA94 advances by 0.013 times
    // the SAVED pre-finale SFX master, not dt and not the current fade. Native
    // settings expose the same normalized master through audioScale().
    const float savedSfxMaster=LegacySettingsTrace::audioScale(settings_.sfx);
    finalScene_.rotorPhase+=0.013000000268220901f*savedSfxMaster;
    while(finalScene_.rotorPhase>=kTwoPi)finalScene_.rotorPhase-=kTwoPi;

    // 0x41B5F7..0x41BD5F: the finale returns only once scene light crosses
    // below zero. r246 DIRECT EXE 0x41A928..0x41A938 then returns the current
    // score 0x257DA20 to wrapper 0x424F48, which opens 0x40F160(score,mode).
    if(finalScene_.sceneLight255<0.0f)
        enterPostGameRecords(static_cast<std::uint32_t>(std::max(0,score_)));
}



void Game::beginPause(){
    // 0x41DE30 pause modal. Stop retained pickup/tick voices, play clc2, then
    // animate cinematic slot 2 / PAUS from Y=-0.3 to -0.03.
    paused_=true;
    pauseLatch_=true;
    pauseScene_={};
    pauseScene_.stage=PauseSceneState::Stage::Entering;
    pauseScene_.panelY=-0.30000001192092896f;
    for(int i=0;i<6;++i)
        queueDeathSpatialStop(static_cast<DeathAudioVoiceTag>(static_cast<int>(DeathAudioVoiceTag::Pickup0)+i));
    if(lowTimeWarningActive_){queueDeathSpatialStop(DeathAudioVoiceTag::LowTimeWarning);lowTimeWarningActive_=false;}
    queueDeathSimple(0x16u); // clc2, 0x41DE94
}

void Game::updatePause(const InputState& input,int dtMs){
    if(input.quit){quit_=true;return;}
    const float dt=float(std::max(0,dtMs));
    pauseScene_.rotorPhase+=dt*0.005f; // 0x41DED8, presentation rotation accumulator
    while(pauseScene_.rotorPhase>=6.2831854820251465f)pauseScene_.rotorPhase-=6.2831854820251465f;

    if(pauseScene_.stage==PauseSceneState::Stage::Entering){
        pauseScene_.panelY+=dt*0.0005000000237487257f;
        if(pauseScene_.panelY>=-0.029999999329447746f){
            pauseScene_.panelY=-0.029999999329447746f;
            pauseScene_.stage=PauseSceneState::Stage::Holding;
            pauseScene_.inputArmed=false; // 0x4093E0 queue clear
        }
        return;
    }
    if(pauseScene_.stage==PauseSceneState::Stage::Holding){
        const bool any=input.pause||input.action||input.back;
        if(!pauseScene_.inputArmed){
            if(!any)pauseScene_.inputArmed=true;
            return;
        }
        if(any){
            queueDeathSimple(0x16u); // clc2, 0x41DF7C
            pauseScene_.stage=PauseSceneState::Stage::Leaving;
        }
        return;
    }
    pauseScene_.panelY-=dt*0.00039999998989515007f;
    if(pauseScene_.panelY<-0.25f){
        paused_=false;
        pauseLatch_=input.pause;
    }
}

void Game::beginAbortConfirm(){
    // 0x41E520 modal ABOR loop. It starts at Y=-0.25, slides to -0.03 at
    // 0.0008/ms, then accepts Yes/Enter or No/Escape. Native action maps to
    // Yes/Enter and Back maps to No/Escape.
    phase_=GamePhase::Abort;
    paused_=false;
    pauseLatch_=false;
    abortConfirm_={};
    abortConfirm_.panelY=-0.25f;
    abortConfirm_.stage=AbortConfirmState::Stage::Entering;
    // 0x41E538..0x41E57B: kill retained pickup/tick voices before the modal.
    for(int i=0;i<6;++i)queueDeathSpatialStop(static_cast<DeathAudioVoiceTag>(static_cast<int>(DeathAudioVoiceTag::Pickup0)+i));
    if(lowTimeWarningActive_){queueDeathSpatialStop(DeathAudioVoiceTag::LowTimeWarning);lowTimeWarningActive_=false;}
    queueDeathSimple(0x16u); // clc2 at 0x41E582
}

void Game::updateAbortConfirm(const InputState& input,int dtMs){
    if(input.quit){quit_=true;return;}
    const float dt=float(std::max(0,dtMs));
    if(abortConfirm_.stage==AbortConfirmState::Stage::Entering){
        abortConfirm_.panelY+=dt*0.0007999999797903001f; // 0x43B428
        if(abortConfirm_.panelY>=-0.03f){
            abortConfirm_.panelY=-0.03f;                  // 0x43B520
            abortConfirm_.stage=AbortConfirmState::Stage::Holding;
        }
        // DIRECT EXE 0x41E653..0x41E664: input polling begins as soon as the
        // entering panel reaches Y >= -0.035, before it settles at -0.03.
        // Keep the native release-arm shim only to emulate the legacy
        // edge-triggered key queue; it must not delay polling until Holding.
        if(abortConfirm_.panelY<-0.03500000014901161f)return; // 0x43B58C
    }
    if(abortConfirm_.stage!=AbortConfirmState::Stage::Leaving){
        if(!abortConfirm_.inputArmed){
            if(!input.action && !input.back && !input.select && !input.pause)abortConfirm_.inputArmed=true;
            return;
        }
        if(input.action){
            abortConfirm_.confirmed=true;
            abortConfirm_.stage=AbortConfirmState::Stage::Leaving;
            queueDeathSimple(0x16u);
        }else if(input.back || input.legacyPressedCode==0x1B){
            abortConfirm_.confirmed=false;
            abortConfirm_.stage=AbortConfirmState::Stage::Leaving;
            queueDeathSimple(0x16u);
        }
        return;
    }
    // DIRECT EXE 0x41E5FB..0x41E626: symmetric 0.0008/ms slide-out, with
    // return only after the panel becomes strictly lower than -0.25.
    abortConfirm_.panelY-=dt*0.0007999999797903001f;
    if(abortConfirm_.panelY<-0.25f){
        if(abortConfirm_.confirmed)enterMainMenu();
        else phase_=GamePhase::Gameplay;
    }
}

void Game::beginPresentation(GamePhase phase){
    phase_=phase;
    paused_=false;
    pauseLatch_=false;
    presentationArmed_=false;
}

void Game::enterRecords(){
    // 0x412F90 entry 2 calls 0x40F160 with the configured mode global.
    // DIRECT EXE 0x40F3A8..0x40F3E1: normal browse starts transition=0,
    // rate=+3 and background phase=0 before entering the frame loop.
    phase_=GamePhase::Records; paused_=false; pauseLatch_=false;
    records_={};
    records_.mode=std::min<std::size_t>(mode_,LegacyRecordsTransition::ModeCount-1u);
    // 0x40F181 -> 0x40F0A0: even ordinary browsing renders a working copy.
    records_.workingBlock=highScores_.blocks[records_.mode];
    records_.inputLatched=true; // 0x4093E0 flush + fresh-key gate at full fade-in
}

void Game::enterPostGameRecords(std::uint32_t score){
    // 0x424F46..0x424F4D calls 0x40F160(score,mode) after gameplay returns a
    // non--1 result. Qualification/insertion occurs in the temporary block.
    phase_=GamePhase::Records; paused_=false; pauseLatch_=false;
    records_={};
    records_.postGame=true;
    records_.mode=std::min<std::size_t>(mode_,LegacyRecordsTransition::ModeCount-1u);
    records_.workingBlock=highScores_.blocks[records_.mode];
    records_.candidateRow=airxonix::insertLegacyHighScoreCandidate(records_.workingBlock,score);
    records_.nameEntry=records_.candidateRow>=0;
    records_.typedNameLength=0;
    records_.inputLatched=true;
}

void Game::updateRecords(const InputState& input,int dtMs){
    if(input.quit){quit_=true;return;}
    using namespace LegacyRecordsTransition;
    // 0x40F3FA occurs before the name-entry transition/input update each frame.
    records_.headingPhase=advanceHeadingPhase(records_.headingPhase,dtMs);
    if(records_.nameEntry)records_.namePulsePhase=advanceNamePulsePhase(records_.namePulsePhase,dtMs);
    records_.fadeCounter=advanceCounter(records_.fadeCounter,records_.fadeRate,dtMs);

    // 0x40F940 -> 0x40FF50: only a negative counter terminates Records. A
    // confirmed post-game record is persisted here, not at Enter time.
    if(records_.fadeRate<0){
        if(records_.fadeCounter>=0)return;
        if(records_.dirty)saveLegacyHighScoresNow();
        enterMainMenu();
        return;
    }

    // Both the name-editor path (0x40F4B8) and browse path (0x40F692) clamp
    // only after the value becomes > 0x7C0 and flush old input once.
    if(records_.fadeCounter>FadeMax){
        records_.fadeCounter=FadeMax;
        records_.readyForInput=true;
    }
    if(!records_.readyForInput)return;

    if(records_.nameEntry){
        auto& name=records_.workingBlock.names[static_cast<std::size_t>(records_.candidateRow)];

        auto applyEdit=[&](int code){
            const auto edit=airxonix::applyLegacyHighScoreNameInput(name,records_.typedNameLength,code);
            if(edit.sfx>=0)queueDeathSimple(static_cast<std::size_t>(edit.sfx));
            if(edit.action==airxonix::LegacyHighScoreNameAction::Cancelled){
                // 0x40F5CB..0x40F5D9 clears qualification + dirty and starts -4.
                records_.nameEntry=false;
                records_.candidateRow=-1;
                records_.dirty=false;
                records_.fadeRate=FadeOutRate;
                return true;
            }
            if(edit.action==airxonix::LegacyHighScoreNameAction::Confirmed){
                // 0x40F620 clears the entry flag, then 0x40F628 -> 0x40F0D0 copies
                // the complete working 200-byte block back to persistent storage.
                highScores_.blocks[records_.mode]=records_.workingBlock;
                records_.nameEntry=false;
                records_.dirty=true;
                records_.inputLatched=true;
                return true;
            }
            return false;
        };

        // r241/r243: preserve the exact PC keyboard editor. SDL_TEXTINPUT has
        // priority so case/layout/punctuation/Cyrillic survive. Any keyboard
        // edit switches the cursor display back to the original append cursor.
        const int code=input.legacyTextByte>=0
            ? input.legacyTextByte
            : (input.legacyPressedFromController ? -1 : input.legacyPressedCode);
        if(code>0){
            const int beforeLength=records_.typedNameLength;
            if(applyEdit(code))return;
            if(records_.typedNameLength!=beforeLength || code==0x08){
                records_.dpadNameEditing=false;
                records_.nameCursor=std::clamp(records_.typedNameLength,0,15);
            }
            return;
        }

        // r247 PortMaster/Anbernic extension. The original game requires a PC
        // keyboard for name entry; handhelds do not have one. D-Pad edits the
        // same fixed 16-byte CP1251 record without changing file compatibility:
        // Up/Down cycles . + А..Я, Left/Right selects a cell, A confirms.
        // B retains the original Escape semantics (it cancels only an empty
        // name). A release latch prevents held directions from racing letters.
        const bool padAny=input.up||input.down||input.left||input.right||input.action||input.back;
        if(!padAny){records_.inputLatched=false;return;}
        if(records_.inputLatched)return;
        records_.inputLatched=true;
        records_.dpadNameEditing=true;

        if(input.left){
            records_.nameCursor=std::max(0,records_.nameCursor-1);
            queueDeathSimple(static_cast<std::size_t>(NavigateSfx));
            return;
        }
        if(input.right){
            records_.nameCursor=std::min(15,records_.nameCursor+1);
            queueDeathSimple(static_cast<std::size_t>(NavigateSfx));
            return;
        }
        if(input.up||input.down){
            const std::size_t at=static_cast<std::size_t>(std::clamp(records_.nameCursor,0,15));
            name[at]=airxonix::cycleLegacyHighScoreDpadChar(name[at],input.up?+1:-1);
            records_.typedNameLength=airxonix::legacyHighScoreVisibleNameLength(name);
            queueDeathSimple(static_cast<std::size_t>(NavigateSfx));
            return;
        }
        if(input.action){
            applyEdit(0x0D); // original Enter confirmation/trim rules
            return;
        }
        if(input.back){
            applyEdit(0x1B); // original Escape cancellation rule for empty name
            return;
        }
        return;
    }

    const bool any=input.action||input.back||input.pause||input.left||input.right||input.up||input.down;
    if(!any){records_.inputLatched=false;return;}
    if(records_.inputLatched)return;
    records_.inputLatched=true;

    // 0x40F71C..0x40F878: Left/Down select previous, Right/Up select next,
    // both wrapping through the configured five game modes and playing 0x15.
    if(input.left||input.down){
        records_.mode=previousMode(records_.mode);
        records_.workingBlock=highScores_.blocks[records_.mode]; // 0x40F772 -> 0x40F0A0
        queueDeathSimple(NavigateSfx);
        return;
    }
    if(input.right||input.up){
        records_.mode=nextMode(records_.mode);
        records_.workingBlock=highScores_.blocks[records_.mode]; // 0x40F880 -> 0x40F0A0
        queueDeathSimple(NavigateSfx);
        return;
    }
    // 0x40F6BD..0x40F6E1: Esc/Backspace/Enter play 0x16 and start -4*dt;
    // return to M1 is delayed until the counter becomes strictly negative.
    if(input.action||input.back||input.pause){
        records_.fadeRate=FadeOutRate;
        queueDeathSimple(ExitSfx);
        return;
    }
}

void Game::enterInformation(){
    // DIRECT EXE 0x410CF0. The Information entry is not one screen: it runs
    // 0x4101B0 (rules), 0x4104A0 (objects/bonuses), then 0x414170
    // (additional enemies), each waiting for a fresh input event.
    phase_=GamePhase::Information;
    paused_=false;
    pauseLatch_=false;
    information_={};
    information_.inputLatched=true; // consume the MainMenu confirm; original waits for a fresh event
    // r201 DIRECT EXE 0x410CF0..0x410D01:
    //   0x40B260(0.001) first fades/stops the current M1 track, then
    //   0x40B240(7,0.001) queues MUSIC\07.mus.  The fade is essential:
    //   0x40B240 alone only installs a pending track and cannot replace an
    //   already-active stream.
    queueDeathMusicFade(0.0010000000474974513f);
    queueMusicRequest(7u,0.0010000000474974513f);
}

void Game::updateInformation(const InputState& input,int dtMs){
    if(input.quit){quit_=true;return;}
    using namespace LegacyInformationTransition;
    const int dt=std::max(0,dtMs);
    information_.promptPhase=advancePromptPhase(information_.promptPhase,dt);
    information_.fadeCounter=advanceCounter(information_.fadeCounter,information_.fadeRate,dt);

    if(information_.fadeRate<0){
        // 0x410361 / 0x4105C3 / 0x414297: the routine returns only after the
        // -4*dt fade has taken the counter below zero.
        if(information_.fadeCounter>=0)return;
        if(information_.page<2){
            ++information_.page;
            information_.fadeCounter=0;
            information_.fadeRate=FadeInRate;
            information_.promptPhase=0; // r331: each 0x4101B0/0x4104A0/0x414170 local starts at zero
            information_.readyForInput=false;
            information_.inputLatched=true;
            return;
        }
        // r201 DIRECT EXE 0x410D3F..0x410D50: leaving the third page first
        // calls 0x40B260(0.002), then requests M1 track 0 with fade-in 0.0015.
        queueDeathMusicFade(0.0020000000949949026f);
        queueMusicRequest(0u,0.001500000013038516f);
        enterMainMenu();
        return;
    }

    // The x86 branches on counter > 0x7C0, clamps back to 0x7C0 and then
    // flushes the old input queue once before accepting a fresh key.
    if(information_.fadeCounter>FadeMax){
        information_.fadeCounter=FadeMax;
        information_.readyForInput=true;
    }
    if(!information_.readyForInput)return;

    const bool any=input.action||input.back||input.pause||input.left||input.right||input.up||input.down;
    if(!any){information_.inputLatched=false;return;}
    if(information_.inputLatched)return;
    information_.inputLatched=true;
    information_.fadeRate=FadeOutRate;
    // All three page loops call 0x40AE50(0x16) when the fresh key starts fade-out.
    queueDeathSimple(AdvanceSfx);
}

void Game::enterSettings(){
    // r300 DIRECT EXE 0x41369A..0x4136D3: Settings owns a zero-position
    // listener/basis and one retained spatial SFX 7 at z=5.0.  The voice is
    // moved to z=0.1 while the SFX-volume row is selected and otherwise stays
    // at z=5.0; it survives the nested Controls screen and is stopped only
    // when Settings itself exits.
    legacyAudioBasisAngle2_=0;
    if(!settingsTestVoiceActive_){
        queueDeathSpatialStart(DeathAudioVoiceTag::SettingsVoice,0x07u,0.f,0.f,5.f,1.f);
        settingsTestVoiceActive_=true;
    }
    // 0x413670 immediately follows constructor 0x423EE0. As with M1, the
    // constructor consumes the shared legacy RNG via 0x422F40.
    phase_=GamePhase::Settings;
    paused_=false;
    pauseLatch_=false;
    settings_.selected=0;
    settings_.fadeCounter=0;
    settings_.fadeRate=LegacySettingsTrace::fadeInRate;
    settings_.readyForInput=false;
    settings_.pendingTransition=SettingsState::PendingTransition::None;
    settings_.navLatched=false;
    settings_.selectorOffset=0.f;
    settings_.speech=speechEnabled_;
    settings_.themeIndex=kLegacyMenuThemeSelectorTrace.select(menuThemeUsage_,rng_.next());
}

void Game::updateSettings(const InputState& input,int dtMs){
    if(input.quit){
        saveLegacySettingsNow();
        if(settingsTestVoiceActive_){queueDeathSpatialStop(DeathAudioVoiceTag::SettingsVoice);settingsTestVoiceActive_=false;}
        quit_=true;return;
    }
    const int dt=std::max(0,dtMs);
    // 0x4136E6..0x413704 updates the retained Settings test voice every loop.
    queueDeathSpatialUpdate(DeathAudioVoiceTag::SettingsVoice,0.f,0.f,settings_.selected==1?0.1f:5.f);
    for(int i=0;i<8;++i){
        const bool sel=i==settings_.selected;
        settings_.brightness[std::size_t(i)]=LegacyMainMenuRuntime::approach(
            settings_.brightness[std::size_t(i)],sel?1.f:0.6f,float(dt)*0.002f);
        settings_.scale[std::size_t(i)]=LegacyMainMenuRuntime::approach(
            settings_.scale[std::size_t(i)],sel?1.15f:1.f,float(dt)*0.001f);
    }

    // r310 DIRECT EXE 0x413749..0x413763 and 0x413B0C..0x413B5E.
    // Controls/Exit do not switch screens immediately: Settings first runs the
    // same counter backwards at -6/ms. Controls returns to counter=0 with +6/ms.
    if(settings_.pendingTransition!=SettingsState::PendingTransition::None){
        settings_.fadeCounter += dt*LegacySettingsTrace::fadeOutRate;
        if(settings_.fadeCounter>=0)return;
        const auto pending=settings_.pendingTransition;
        settings_.pendingTransition=SettingsState::PendingTransition::None;
        if(pending==SettingsState::PendingTransition::Controls){
            enterControlsRemap();
            return;
        }
        speechEnabled_=settings_.speech;
        saveLegacySettingsNow();
        if(settingsTestVoiceActive_){queueDeathSpatialStop(DeathAudioVoiceTag::SettingsVoice);settingsTestVoiceActive_=false;}
        enterMainMenu();
        return;
    }

    // r309 DIRECT EXE 0x413704..0x413769. The menu is visible while the
    // 3-D scene fades in, but the legacy event queue is not accepted until the
    // counter passes 0x7C0 and is clamped back to it.
    if(!settings_.readyForInput){
        settings_.fadeCounter += dt*settings_.fadeRate;
        if(settings_.fadeCounter>LegacySettingsTrace::fadeMax){
            settings_.fadeCounter=LegacySettingsTrace::fadeMax;
            settings_.readyForInput=true;
            settings_.navLatched=true; // emulate 0x4093E0 queue clear
        }
        if(!settings_.readyForInput)return;
        // Require release after the queue clear before accepting a fresh event.
        if(input.up||input.down||input.action||input.back||input.left||input.right)return;
        settings_.navLatched=false;
    }

    // r350 DIRECT EXE 0x413769..0x413BFA: the event queue is not polled
    // while the selected-row camera slide is still travelling. The current
    // offset approaches -selected*0.0028 at 0.00005/ms; after a fresh row
    // event the first movement step happens in the same frame.
    // Native lives/time controls are displayed in the upper right, ABOVE the
    // six original rows. They must not push the original 3D camera down beyond
    // the Exit row (the r366 bug moved the menu off-screen on selection 6/7).
    const auto settingsCameraTarget=[&](){
        return LegacySettingsTrace::selectorTarget(settings_.selected>=6?0:settings_.selected);
    };
    const auto advanceSelector=[&](){
        settings_.selectorOffset=LegacyMainMenuRuntime::approach(
            settings_.selectorOffset,settingsCameraTarget(),
            float(dt)*LegacySettingsTrace::selectorRatePerMs);
    };
    const float selectorTarget=settingsCameraTarget();
    if(std::fabs(settings_.selectorOffset-selectorTarget)>1.0e-7f){
        advanceSelector();
        // The legacy event queue is simply not polled during the slide. Our
        // key-edge latch must still observe releases here, otherwise the event
        // that initiated the slide would remain latched after motion ends.
        if(!(input.up||input.down||input.action||input.back)) settings_.navLatched=false;
        if(!(input.left||input.right)) settings_.speechLrLatched=false;
        return;
    }

    // 0x4137FE..0x413A93: sliders are continuous while Left/Right is held.
    const float delta=float(dt)*0.6000000238418579f;
    auto adjust=[&](float& v){
        if(input.left)v=std::max(0.f,v-delta);
        if(input.right)v=std::min(1000.f,v+delta);
    };
    if(settings_.selected==0)adjust(settings_.speed);
    else if(settings_.selected==1)adjust(settings_.sfx);
    else if(settings_.selected==2)adjust(settings_.music);

    // r311 DIRECT EXE 0x413AAB..0x413AD9 and 0x413ADC..0x413B2F:
    // Speech is a binary row and both Left and Right toggle it. The legacy key
    // queue is edge/event based, so a held direction must not toggle every frame.
    if(settings_.selected==3){
        const bool lr=input.left||input.right;
        if(!lr) settings_.speechLrLatched=false;
        else if(!settings_.speechLrLatched){
            settings_.speechLrLatched=true;
            settings_.speech=!settings_.speech;
            speechEnabled_=settings_.speech;
            queueDeathSimple(0x16u);
        }
    }else settings_.speechLrLatched=false;

    // Native debug rows. Holding D-Pad left/right now repeats instead of
    // requiring one release+press for every step. The first step is immediate,
    // then repeat starts after 320 ms at 90-ms cadence (handheld-friendly).
    if(settings_.selected==6 || settings_.selected==7){
        const int dir=input.right?1:(input.left?-1:0);
        if(dir==0){ settings_.testLrLatched=false; settings_.testRepeatMs=0; }
        else {
            bool fire=false;
            if(!settings_.testLrLatched){ fire=true; settings_.testLrLatched=true; settings_.testRepeatMs=-320; }
            else { settings_.testRepeatMs+=dt; if(settings_.testRepeatMs>=0){ fire=true; settings_.testRepeatMs-=90; } }
            if(fire){
                if(settings_.selected==6) settings_.testInitialLives=std::clamp(settings_.testInitialLives+dir,1,99);
                else settings_.testInitialTimeSeconds=std::clamp(settings_.testInitialTimeSeconds+dir*10,10,600);
            }
        }
    }else{ settings_.testLrLatched=false; settings_.testRepeatMs=0; }

    const bool nav=input.up||input.down||input.action||input.back;
    if(!nav){settings_.navLatched=false;return;}
    if(settings_.navLatched)return;
    settings_.navLatched=true;
    if(input.up){
        const int old=settings_.selected;
        // Above SPEED are TIME and LIVES. UP from SPEED reaches this mod panel.
        settings_.selected=(old==0?7:(old==7?6:std::max(0,old-1)));
        if(settings_.selected!=old){queueDeathSimple(0x15u);advanceSelector();}
        return;
    }
    if(input.down){
        const int old=settings_.selected;
        // DOWN from LIVES -> TIME -> SPEED; EXIT remains the last original row.
        settings_.selected=(old==6?7:(old==7?0:std::min(5,old+1)));
        if(settings_.selected!=old){queueDeathSimple(0x15u);advanceSelector();}
        return;
    }
    if(input.back){
        settings_.selected=5;
        advanceSelector();
        settings_.readyForInput=false;
        settings_.pendingTransition=SettingsState::PendingTransition::Exit;
        settings_.fadeRate=LegacySettingsTrace::fadeOutRate;
        queueDeathSimple(0x16u);
        return;
    }
    if(input.action){
        if(settings_.selected==3){settings_.speech=!settings_.speech;speechEnabled_=settings_.speech;queueDeathSimple(0x16u);return;}
        if(settings_.selected==4){
            settings_.readyForInput=false;
            settings_.pendingTransition=SettingsState::PendingTransition::Controls;
            settings_.fadeRate=LegacySettingsTrace::fadeOutRate;
            queueDeathSimple(0x16u);
            return;
        }
        if(settings_.selected==5){
            settings_.readyForInput=false;
            settings_.pendingTransition=SettingsState::PendingTransition::Exit;
            settings_.fadeRate=LegacySettingsTrace::fadeOutRate;
            queueDeathSimple(0x16u);
            return;
        }
    }
}

void Game::enterControlsRemap(){
    phase_=GamePhase::Controls;
    controlsRemap_.temporary=settings_.bindings;
    controlsRemap_.assigned=0;
    controlsRemap_.awaitingConfirm=false;
    controlsRemap_.fadeCounter=0;
    controlsRemap_.fadeRate=kLegacyControlsTrace.fadeInRate;
    controlsRemap_.readyForInput=false;
    controlsRemap_.exitPending=false;
    controlsRemap_.commitOnExit=false;
    controlsRemap_.pulsePhase=0.f;
    // 0x410D7D clears the legacy event queue before accepting assignments.
    settings_.navLatched=true;
    // r201 DIRECT EXE 0x410E30..0x410E35: once the four current binding
    // labels have been prepared, the controls screen fades the M1 music with
    // exactly 0.001/ms and does not request a replacement track while open.
    queueDeathMusicFade(0.0010000000474974513f);
}

void Game::updateControlsRemap(const InputState& input,int dtMs){
    if(input.quit){quit_=true;return;}

    // r315 DIRECT EXE 0x41103E..0x4110D8: the current assignment row and
    // instruction/confirmation line pulse continuously even during menu fades.
    controlsRemap_.pulsePhase=kLegacyControlsTrace.advancePulse(controlsRemap_.pulsePhase,dtMs);

    // r313 DIRECT EXE 0x410FED..0x411030 and 0x411291..0x4112D8.
    controlsRemap_.fadeCounter += std::max(0,dtMs)*controlsRemap_.fadeRate;
    if(controlsRemap_.fadeCounter>kLegacyControlsTrace.fadeMax){
        controlsRemap_.fadeCounter=kLegacyControlsTrace.fadeMax;
        controlsRemap_.readyForInput=true;
        // 0x411011 clears the event queue once the enter fade has completed.
        settings_.navLatched=true;
    }
    if(controlsRemap_.fadeCounter<0){
        if(controlsRemap_.commitOnExit){
            settings_.bindings=controlsRemap_.temporary;
            // r317 DIRECT EXE 0x411297..0x4112C5: committing the four
            // temporary bindings immediately persists the settings block.
            saveLegacySettingsNow();
        }
        // 0x4112CA..0x4112D0: commit and cancel both restore fixed M1 track 0.
        queueMusicRequest(0u,0.0010000000474974513f);
        phase_=GamePhase::Settings;
        settings_.fadeCounter=0;
        settings_.fadeRate=LegacySettingsTrace::controlsReturnFadeInRate;
        settings_.readyForInput=false;
        settings_.pendingTransition=SettingsState::PendingTransition::None;
        settings_.navLatched=true;
        return;
    }
    if(controlsRemap_.exitPending || !controlsRemap_.readyForInput)return;

    // 0x41110B: Escape starts the -4/ms exit with commit flag clear.
    if(input.back || input.legacyPressedCode==0x1B){
        queueDeathSimple(0x16u);
        controlsRemap_.exitPending=true;
        controlsRemap_.commitOnExit=false;
        controlsRemap_.readyForInput=false;
        controlsRemap_.fadeRate=kLegacyControlsTrace.fadeOutRate;
        return;
    }
    if(controlsRemap_.awaitingConfirm){
        // 0x41112F..0x411155: only Enter commits once all four fields exist,
        // and the persistent bindings are copied only after fade-out completes.
        if(input.legacyPressedCode==0x0D || input.action){
            queueDeathSimple(0x16u);
            controlsRemap_.exitPending=true;
            controlsRemap_.commitOnExit=true;
            controlsRemap_.readyForInput=false;
            controlsRemap_.fadeRate=kLegacyControlsTrace.fadeOutRate;
        }
        return;
    }
    const int code=input.legacyPressedCode;
    if(!kLegacyControlsTrace.assignable(code))return;
    for(int i=0;i<controlsRemap_.assigned;++i)
        if(controlsRemap_.temporary[static_cast<std::size_t>(i)]==code)return;
    controlsRemap_.temporary[static_cast<std::size_t>(controlsRemap_.assigned)]=code;
    // r314 DIRECT EXE 0x4111E8..0x4111F0: only an accepted assignment
    // advances the field and plays navigation/accept SFX 0x15.
    queueDeathSimple(0x15u);
    ++controlsRemap_.assigned;
    controlsRemap_.awaitingConfirm=controlsRemap_.assigned==4;
}

void Game::enterMainMenu(){
    // Direct trace of 0x00412F90: five entries, selected index 0..4.
    phase_=GamePhase::MainMenu;
    paused_=false;
    pauseLatch_=false;
    presentationArmed_=false;
    // The highlight arrays are global storage in the x86 build and are only
    // initialized once in .data; do not reset them when returning to the menu.
    mainMenu_.selected=0;
    mainMenu_.inputLatched=false;
    mainMenu_.exitPending=false;
    mainMenu_.exitDelayMs=0;
    mainMenu_.hasPendingDispatch=false;
    mainMenu_.pendingDispatch=kLegacyMainMenuEntries[0];
    // 0x422F40 is called by the M1/M2 constructor and consumes the shared MSVC
    // rand() stream. Preserve that call here so later gameplay RNG ordering is
    // not silently decoupled from menu construction.
    mainMenu_.themeIndex=kLegacyMenuThemeSelectorTrace.select(menuThemeUsage_,rng_.next());
}

void Game::updateMainMenu(const InputState& input,int dtMs){
    if(mainMenu_.exitPending){
        mainMenu_.exitDelayMs-=std::max(0,dtMs);
        if(mainMenu_.exitDelayMs<=0)quit_=true;
        return;
    }
    if(dtMs>0){
        for(int i=0;i<5;++i){
            const bool selected=(i==mainMenu_.selected);
            mainMenu_.brightness[static_cast<std::size_t>(i)]=LegacyMainMenuRuntime::approachBrightness(
                mainMenu_.brightness[static_cast<std::size_t>(i)],selected,dtMs);
            mainMenu_.scale[static_cast<std::size_t>(i)]=LegacyMainMenuRuntime::approachScale(
                mainMenu_.scale[static_cast<std::size_t>(i)],selected,dtMs);
        }
        mainMenu_.selectorOffset=kLegacyMainMenuSelectorSlideTrace.advance(
            mainMenu_.selectorOffset,mainMenu_.selected,dtMs);
    }
    if(input.quit){quit_=true;return;}
    // r184: 0x413074 reaches 0x409410 only when the visual row coordinate
    // exactly matches selected*(-.003). During the slide, new menu events are
    // deliberately ignored.
    const float selectorTarget=kLegacyMainMenuSelectorSlideTrace.targetFor(mainMenu_.selected);
    const bool any=input.up||input.down||input.action||input.back||input.right;
    // Observe releases while a row is sliding. Previously the latch remained
    // stuck from the previous press, so subsequent touches were discarded.
    if(std::fabs(mainMenu_.selectorOffset-selectorTarget)>1.0e-7f){
        if(!any)mainMenu_.inputLatched=false;
        return;
    }
    if(!any){mainMenu_.inputLatched=false;return;}
    if(mainMenu_.inputLatched)return;
    mainMenu_.inputLatched=true;

    // 0x00413090..0x004130C8: Up/Down clamp EBP to [0,4].
    if(input.up){mainMenu_.selected=std::max(0,mainMenu_.selected-1);return;}
    if(input.down){mainMenu_.selected=std::min(4,mainMenu_.selected+1);return;}

    // 0x004131D2..0x004131F2: Escape/Back does not quit immediately; it moves
    // selection to entry 4 unless entry 4 is already selected.
    if(input.back){if(mainMenu_.selected!=4){mainMenu_.selected=4;mainMenu_.selectorOffset=-0.012000000104308128f;}return;}

    // Enter/Space/Right/configured action use the dispatch table. The native
    // port now executes all five proven outer-menu targets: mode selector,
    // settings, records, information, and delayed menu exit.
    if(input.action||input.right){
        mainMenu_.pendingDispatch=kLegacyMainMenuEntries[static_cast<std::size_t>(mainMenu_.selected)];
        mainMenu_.hasPendingDispatch=true;
        // r81: execute the two menu transitions whose ownership is already
        // proven by the outer dispatcher. Entry 0 enters gameplay; entry 4
        // returns from the menu and therefore exits the native port.
        if(mainMenu_.selected==0){ enterModeSelect(); return; }
        if(mainMenu_.selected==1){ enterSettings(); return; }
        if(mainMenu_.selected==2){ enterRecords(); return; }
        if(mainMenu_.selected==3){ enterInformation(); return; }
        if(mainMenu_.selected==4){
            // WAVEPACK logical 0x28 = "byeb". Let the phrase finish before
            // the process tears down the mixer (13844 samples at 22050 Hz ~628 ms).
            queueDeathSimple(0x28u);
            mainMenu_.exitPending=true;
            mainMenu_.exitDelayMs=700;
            return;
        }
    }
}


void Game::enterModeSelect(){
    // r305 DIRECT EXE registered selector 0x411870. The selected row comes
    // from persisted 0x25B7884 (gameinf.bin +0x0C), then the screen fades in
    // 0 -> 0x7C0 at +3/ms before input is accepted.
    phase_=GamePhase::ModeSelect;
    paused_=false;
    pauseLatch_=false;
    modeSelect_={};
    modeSelect_.selected=std::clamp<int>(legacySelectedMode_,0,4);
    modeSelect_.inputLatched=true; // matches 0x4093E0 fresh-key gate at fade-in completion.
}

void Game::updateModeSelect(const InputState& input,int dtMs){
    if(input.quit){quit_=true;return;}
    const int dt=std::max(0,dtMs);
    // r306 DIRECT EXE 0x411B15..0x411C7C. These presentation phases advance
    // on every rendered selector frame, including fade-in and fade-out.
    modeSelect_.anglePhase+=dt*2;
    modeSelect_.backgroundPhase+=float(dt)*0.0005000000237487257f;
    while(modeSelect_.backgroundPhase>=1.f)modeSelect_.backgroundPhase-=1.f;
    if(modeSelect_.exitPending){
        modeSelect_.fadeCounter+=dt*modeSelect_.fadeRate;
        if(modeSelect_.fadeCounter<0){
            if(modeSelect_.cancelPending){ enterMainMenu(); }
            else {
                legacySelectedMode_=modeSelect_.selected; // 0x411D3E -> 0x25B7884
                saveLegacySettingsNow();                 // 0x411D44 -> 0x4230A0
                startNewSession(static_cast<std::size_t>(modeSelect_.selected));
            }
        }
        return;
    }
    if(!modeSelect_.readyForInput){
        modeSelect_.fadeCounter+=dt*modeSelect_.fadeRate;
        if(modeSelect_.fadeCounter>0x7c0){
            modeSelect_.fadeCounter=0x7c0;
            modeSelect_.readyForInput=true;
            // A held confirmation from the prior menu must not auto-confirm,
            // but a fully released control must be armed immediately.
            modeSelect_.inputLatched=input.up||input.down||input.action||
                input.back||input.left||input.right;
        }
        return;
    }
    const bool any=input.up||input.down||input.action||input.back||input.left||input.right;
    if(!any){modeSelect_.inputLatched=false;return;}
    if(modeSelect_.inputLatched)return;
    modeSelect_.inputLatched=true;
    if(input.up||input.left){
        if(modeSelect_.selected>0){--modeSelect_.selected;queueDeathSimple(0x15u);}
        return;
    }
    if(input.down||input.right){
        if(modeSelect_.selected<4){++modeSelect_.selected;queueDeathSimple(0x15u);}
        return;
    }
    if(input.back){
        queueDeathSimple(0x16u); modeSelect_.exitPending=true;modeSelect_.cancelPending=true;modeSelect_.fadeRate=-4;return;
    }
    if(input.action){
        queueDeathSimple(0x16u); modeSelect_.exitPending=true;modeSelect_.cancelPending=false;modeSelect_.fadeRate=-4;return;
    }
}

void Game::updatePresentation(const InputState& input){
    if(input.quit){quit_=true;return;}
    // Require a full release before accepting the confirmation. This prevents
    // the Back press that opened Abort from also closing it in the same frame.
    if(!presentationArmed_){
        if(!input.action && !input.back && !input.pause)presentationArmed_=true;
        return;
    }
    if(input.action || input.back || input.pause)enterMainMenu();
}

void Game::update(const InputState& input,int dtMs){
    // r298 DIRECT EXE 0x424818..0x424856: these display counters are updated
    // by the HUD renderer after the current frame's gameplay mutations, not
    // before them. Keep the update as a scope-exit so every early-return phase
    // still gets the same post-state HUD step.
    ScopeExit hudPost{[&]{
        if(dtMs<=0)return;
        const int step=dtMs*32;
        if(hudDisplayTimer_<timer_) hudDisplayTimer_=std::min(timer_,hudDisplayTimer_+step);
        else if(hudDisplayTimer_>timer_) hudDisplayTimer_=std::max(timer_,hudDisplayTimer_-step);
        if(hudDisplayScore_<score_) hudDisplayScore_=std::min(score_,hudDisplayScore_+step);
        else if(hudDisplayScore_>score_) hudDisplayScore_=score_;
        hudPulseCounter_=(hudPulseCounter_+dtMs)&0x7f;
    }};
    if(phase_==GamePhase::MainMenu){updateMainMenu(input,dtMs);return;}
    if(phase_==GamePhase::ModeSelect){updateModeSelect(input,dtMs);return;}
    if(phase_==GamePhase::Records){updateRecords(input,dtMs);return;}
    if(phase_==GamePhase::Information){updateInformation(input,dtMs);return;}
    if(phase_==GamePhase::Settings){updateSettings(input,dtMs);return;}
    if(phase_==GamePhase::Controls){updateControlsRemap(input,dtMs);return;}
    if(phase_==GamePhase::GameOver){updateGameOverTail(input,dtMs);return;}
    if(phase_==GamePhase::Abort){updateAbortConfirm(input,dtMs);return;}
    if(phase_==GamePhase::Complete){updatePresentation(input);return;}
    if(input.quit){quit_=true;return;}
    if(levelIntroScene_.active()){updateLevelIntro(dtMs);return;}
    if(levelEntryScene_.active()){updateLevelEntry(dtMs);return;}
    if(phase_==GamePhase::FinalSequence){updateFinalSequence(input,dtMs);return;}
    if(paused_){ updatePause(input,dtMs); return; }
    // r185 DIRECT EXE 0x419270: Backspace is a separate restart-current-level
    // command, not the Esc/ABOR path. It is edge-triggered and available only
    // while one of the five campaign restart credits remains.
    if(input.legacyPressedCode==kLegacyRestartCurrentLevelTrace.keyboardKey && restartCredits_>0){
        restartCurrentLevel();
        return;
    }

    // Native PortMaster mapping: Select replaces the original PC Esc request.
    // Keep keyboard Esc as a compatibility trigger, while controller B is reserved
    // for Cancel inside the confirmation modal.
#if defined(__ANDROID__)
    // Touchscreen has no dedicated Select button: B opens the same abort
    // confirmation and still cancels when that confirmation is visible.
    if(input.select || input.back || input.legacyPressedCode==0x1B){beginAbortConfirm();return;}
#else
    if(input.select || input.legacyPressedCode==0x1B){beginAbortConfirm();return;}
#endif
    if(input.pause&&!pauseLatch_){ beginPause(); return; }
    pauseLatch_=input.pause;
    // r292 DIRECT EXE 0x4192F6..0x419318: keyboard 'M' (0x4D) fades
    // the current track, chooses another least-used gameplay track through
    // 0x422EE0, and queues it through 0x40B240. It is a keyboard-only PC
    // compatibility hotkey and does not consume the gameplay frame.
    if(!input.legacyPressedFromController && input.legacyPressedCode==0x4D){
        queueDeathMusicFade(0.0010000000474974513f);
        currentMusicTrack_=airxonix::LegacyMusicSelectorTrace::select(musicTrackUsage_,rng_.next());
        queueMusicRequest(currentMusicTrack_,0.0005000000237487257f);
    }
    if(dtMs<=0)return;

    if(phase_==GamePhase::Dying){updateDeathSequence(dtMs);return;}
    if(phase_==GamePhase::InterLevel){updateInterLevel(dtMs);return;}

    // Safety for restored/legacy states that enter Gameplay without the intro.
    if(levelMusicPending_)selectLevelMusic();
    timer_-=dtMs;
    updateLegacyTimerPressure(dtMs);
    if(timer_<0){
        // 0x419795..0x41979F: direct dispatcher id 5 at Xonix coordinates,
        // then timer clamp/timeout transition. The auxiliary survives into the
        // live death-world loop and rises at half the normal rate.
        spawnAuxiliaryEffect(5,player_.worldX(),player_.visualY(),player_.worldZ());
        timer_=0;
        handleDeath();
        return;
    }

    updateLegacyEffects(dtMs);
    auto seeds=entities_.captureSeeds();
    // r252 DIRECT EXE 0x41848B..0x418515: 0x418430 first flood-clears every
    // airborne-enemy component, then (when 0x254A290 != 0) also the component
    // containing the field eraser. The previous native port omitted this
    // second seed, so a region containing only the eraser could be captured.
    GridSeed eraserSeed{};
    if(specialObjects_.eraserCaptureSeed(eraserSeed))seeds.push_back(eraserSeed);
    // 0x419270: cursor keys are always accepted; the four configurable
    // globals are ORed into Right/Left/Backward/Forward respectively.
    InputState gameplayInput=input;
    gameplayInput.right = gameplayInput.right || input.legacyDown(settings_.bindings[0]);
    gameplayInput.left  = gameplayInput.left  || input.legacyDown(settings_.bindings[1]);
    gameplayInput.down  = gameplayInput.down  || input.legacyDown(settings_.bindings[2]);
    gameplayInput.up    = gameplayInput.up    || input.legacyDown(settings_.bindings[3]);
    player_.update(gameplayInput,dtMs,field_,seeds);
    consumeCaptureStartedAudio();
    player_.advanceTrailPresentation(dtMs);

    // Main gameplay, 0x41A12E..0x41A18C:
    // DA94 += dt * (0.007 + (cutting ? 0.006 : 0));
    // DA98  = cutting ? 0.0037 : 0.0047.
    // The four-part rotor therefore spins faster and retracts while Xonix is
    // cutting through unclaimed territory.
    playerRotorPhase_+=float(dtMs)*(0.007f+(player_.cutting()?0.006f:0.0f));
    while(playerRotorPhase_>=kTwoPi)playerRotorPhase_-=kTwoPi;
    playerRotorRadius_=player_.cutting()?0.0037f:0.0047f;

    // DIRECT EXE 0x419FED..0x41A129 + 0x420BB0. The central propeller is
    // independent of the four-node orbit: trailActive selects dt*.04 vs
    // dt*.004, acceleration is immediate, deceleration is dt*.0006, and the
    // per-frame step is clamped to pi/2 before subtraction from 0x25B5B34.
    const float propTarget=float(dtMs)*(player_.cutting()?0.04f:0.004f);
    if(playerPropellerStep_>propTarget)
        playerPropellerStep_=std::max(propTarget,playerPropellerStep_-float(dtMs)*0.0006000000284984708f);
    else
        playerPropellerStep_=propTarget;
    playerPropellerStep_=std::min(playerPropellerStep_,kPi*0.5f);
    playerPropellerPhase_-=playerPropellerStep_;
    while(playerPropellerPhase_<0.f)playerPropellerPhase_+=kTwoPi;

    field_.updateCaptureAnimations(dtMs);

    // 0x419D0A..0x419DDB occurs BEFORE the 0x416110/0x416950 world updates.
    // It tests Xonix against each active crawler using exact radius 0.0065,
    // emits the 0x417390 512-particle death burst at Y=0.008, resets the hit
    // crawler to spawn with delay 1.7, then sets the death flag.
    float crawlerHitX=0.f,crawlerHitZ=0.f;
    if(entities_.consumeCrawlerPlayerHit(player_.worldX(),player_.worldZ(),crawlerHitX,crawlerHitZ)){
        // 0x41C0FD..0x41C169: crawler-caused death starts the orange
        // 0x41BDB0 overlay at current Xonix X/Z, fixed Y=.025.
        startDeathOverlay(player_.worldX(),0.02500000037252903f,player_.worldZ(),255.f,50.f,0.f);
        spawnDeathBurst(crawlerHitX,0.008f,crawlerHitZ);
        const float dx=player_.worldX()-crawlerHitX,dz=player_.worldZ()-crawlerHitZ;
        const float d=std::sqrt(dx*dx+dz*dz);
        if(d>1e-8f)setPendingDeathDrift(dx/d*0.00007000000186963007f,dz/d*0.00007000000186963007f);
        player_.kill(); handleDeath(); return;
    }

    // 0x419E05..0x419EFD: the next direct-body lethal consumer belongs to
    // the homing special, not the ordinary airborne enemies. It is checked
    // before the world-update routines, just like the crawler loop above.
    float homingHitX=0.f,homingHitY=0.f,homingHitZ=0.f;
    if(specialObjects_.consumeHomingPlayerHit(player_.worldX(),player_.worldZ(),homingHitX,homingHitY,homingHitZ)){
        spawnDeathBurst(homingHitX,homingHitY,homingHitZ);
        const float dx=player_.worldX()-homingHitX,dz=player_.worldZ()-homingHitZ;
        const float d=std::sqrt(dx*dx+dz*dz);
        if(d>1e-8f)setPendingDeathDrift(dx/d*0.00009999999747378752f,dz/d*0.00009999999747378752f);
        player_.kill(); handleDeath(); return;
    }

    resolveSpecialPreEntityContacts();
    entities_.update(dtMs,field_,rng_,enemySpeedFactor_);
    consumeEntityCollisionAudioEvents();
    // Ordinary airborne enemies have no matching 0x409140 direct-body
    // consumer in the traced gameplay loop. Their confirmed lethal route is
    // trail hazard/propagation, so the old native 0.0095 swept-radius kill
    // has been removed.
    effectEvents_.airErosionImpactEvents+=entities_.consumeAirErosionImpactEvents();
    effectEvents_.airErosionDebrisHelperCalls+=entities_.consumeAirErosionDebrisHelperCalls();
    specialObjects_.update(dtMs,field_,player_,enemySpeedFactor_,rng_);
    consumeSpecialPostUpdateEvents();
    updatePickupRuntime(dtMs,{player_.worldX(),player_.visualY(),player_.worldZ()});
    // 0x419FE8 -> 0x41FD50 follows the shared pickup update.
    advanceLegacyBackgroundScroll(dtMs);

    {
        auto impacts=entities_.consumeTrailImpacts();
        for(const auto& hit:impacts)player_.markTrailHazardNear(hit.x,hit.y);
    }
    // 0x41A68D first marks a strict 3x3 area around the hazard center.
    // 0x41A6E5 then propagates hazard through adjacent trail records every
    // 17 ms; death is therefore chain/cadence driven, not immediate. The eraser keeps its
    // separately confirmed immediate Trail-bit death predicate.
    if(player_.updateTrailHazard(dtMs) || specialObjects_.consumeTrailHit())player_.kill();
    if(player_.dead()){handleDeath();return;}
    updateScoreAndPercent();

    if(capturePercent_>=100 && !player_.cutting()){
        const auto& levels=database_.modes()[mode_].levels;
        if(level_+1<levels.size())beginInterLevel(level_+1);
        else beginFinalSequence();
    }
}

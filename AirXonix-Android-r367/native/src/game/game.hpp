#pragma once
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cstddef>
#include <array>
#include <vector>
#include <utility>
#include <cmath>
#include "entities.hpp"
#include "core/legacy_random.hpp"
#include "core/legacy_highscore.hpp"
#include "field.hpp"
#include "level.hpp"
#include "pickups.hpp"
#include "player.hpp"
#include "special_objects.hpp"
#include "platform/input.hpp"
#include "legacy_menu.hpp"
#include "legacy_particles.hpp"
#include "legacy_settings_trace.hpp"

enum class GamePhase {
    Gameplay,
    Dying,
    InterLevel,
    FinalSequence,
    MainMenu,
    Complete,
    GameOver,
    Abort,
    Records,
    Information,
    Settings,
    Controls,
    ModeSelect
};

struct LevelIntroState {
    int elapsedMs=0;
    int durationMs=3000; // Motion countdown only: 0x41D5C3 / 0x41DC13: EBP=0xBB8.
    // r347 DIRECT EXE 0x41D50E, 0x41D5BB, 0x41D627..0x41D682:
    // the capable presentation owns a 0..255..0 triangular light pulse.
    float lightByte=0.f;
    float presentedLightByte=0.f;
    float lightVelocityPerMs=0.13500000536441803f; // 0x3E0A3D71
    bool finished=false;
    float plaqueY=-0.10000000149011612f; // capable path local initialized at 0x41D5E0.
    float plaqueAngleRad=0.f; // 0x41D5CC / fallback 0x41DC1C: LEV2 Z angle starts at zero.
    float digitLateX=0.f;                // 0x41D5D4; grows only in the final second.
    float worldX=0.50156247615814209f;   // DA48 @ 0x41D536
    float worldY=0.31999999284744263f;   // DA4C @ 0x41D52C
    float worldZ=0.39218750596046448f;   // DA50 @ 0x41D540
    float cameraX=0.5f;                  // DA58
    float cameraY=0.42300000786781311f;  // DA5C
    float cameraZ=0.40000000596046448f;  // DA60
    int cameraAngle1=0,cameraAngle2=-450,cameraAngle3=0; // 0x41D568
    bool cue14Consumed=false;
    bool cue5Consumed=false;
    int remainingMs() const{return durationMs>elapsedMs?durationMs-elapsedMs:0;}
    // The original does not return when EBP reaches zero. It continues the
    // late choreography until the descending light pulse crosses below zero.
    // The second clause preserves old test/probe code that forces gameplay by
    // setting elapsedMs=durationMs on an untouched default state.
    bool active() const{return !finished && (elapsedMs<durationMs || lightByte>0.f || lightVelocityPerMs<0.f);}
    float lightScale() const{return std::clamp(presentedLightByte*(1.f/255.f),0.f,1.f);}
    float oneDigitAdjust(int levelNumber) const{return levelNumber<10?0.0007999999797903001f:0.f;}
    float plaqueX(int levelNumber) const{return oneDigitAdjust(levelNumber)-0.003000000026077032f;}
    float digitX(int levelNumber) const{return oneDigitAdjust(levelNumber)+0.004999999888241291f+(remainingMs()<=1000?digitLateX:0.f);}
    float digitY() const{return remainingMs()>1000?plaqueY:-0.02500000037252903f;}
    float plaqueRotationRad() const{return remainingMs()>1000?0.f:plaqueAngleRad;}
    static constexpr float digitZ=-0.002099999925121665f;
};


struct LevelEntryState {
    bool running=false;
    float light=0.f;                 // 0x41CEA0 second startup scene local
    float worldX=0.50156247615814209f;
    float worldY=0.18999999761581421f;
    float worldZ=0.39218750596046448f;
    float descentPerMs=0.00019999999494757503f;
    float cameraX=0.50078123807907104f;
    float cameraY=0.23039999604225159f;
    float cameraZ=0.34609374403953552f;
    int cameraAngle1=0,cameraAngle2=-344,cameraAngle3=0;
    bool active() const{return running;}
};

struct DeathBurstParticle { float x=0.f,y=0.f,z=0.f,vx=0.f,vy=0.f,vz=0.f; };

enum class DeathAudioEventKind {
    SpatialPlay,
    SpatialStart,
    SpatialUpdate,
    SpatialStop,
    SimplePlay,
    MusicFadeOut,
    MusicRequest
};

enum class DeathAudioVoiceTag { None, InitialDeathVoice, RespawnVoice, InterLevelVoice, LevelEntryVoice, SettingsVoice, LowTimeWarning, Pickup0, Pickup1, Pickup2, Pickup3, Pickup4, Pickup5 };

struct DeathAudioEvent {
    DeathAudioEventKind kind=DeathAudioEventKind::SimplePlay;
    DeathAudioVoiceTag voice=DeathAudioVoiceTag::None;
    std::size_t logicalId=0;
    float x=0.f,y=0.f,z=0.f,scalar=1.f;
    float fadePerMs=0.f;
};

struct DeathOverlayState {
    bool active=false;
    float x=0.f,y=0.f,z=0.f;
    float halfExtent=0.010000000707805157f;
    float intensity=1.f;
    float r=255.f,g=255.f,b=255.f;
};

struct DeathSceneState {
    int elapsedMs=0;
    int durationMs=4000; // CONFIRMED: 0x0041C081 -> 0xFA0 countdown, decremented at 0x0041C1A7.
    float worldX=0.5f,worldY=0.008f,worldZ=0.5f;
    float velocityX=0.f,velocityY=0.00018f,velocityZ=0.f;
    bool triColorBurstStarted=false;
    bool finalSecondRespawnStarted=false;
    bool logicalRespawnPrepared=false; // 0x41C599..0x41C5BA exact timing
    bool initialDeathVoiceStopped=false;
    bool zeroLivesAudioStarted=false;
    bool earlySpeechCueConsumed=false; // 0x41C1B9: one-shot once remaining < 3850 ms.
    float zeroLivesPhase=0.f;
    float zeroLivesPitch=-0.3f; // EXE local +0x18; drives zero-lives grayscale modulation.
    float zeroLivesGray=255.f;
    float rotorPhase=0.f;
    float rotorRadius=0.0057f;
    float cameraShakeAmplitude=0.f;
    float cameraShakePhase=0.f;
    int cameraYawOffset=0;
    int remainingMs() const{return durationMs>elapsedMs?durationMs-elapsedMs:0;}
    bool lastSecond() const{return remainingMs()<1000;} // 0x0041C540 threshold.
};

struct GameOverSceneState {
    // 0x41C6BE..0x41CE87: zero-lives tail remains inside the death routine.
    // The negative death countdown is allowed to run for another 60000 ms and
    // exits on an input/event queue item or timeout.
    int elapsedMs=0;
    int timeoutMs=60000;
    DeathSceneState visual{};
    bool inputArmed=false;
    int remainingMs() const{return timeoutMs>elapsedMs?timeoutMs-elapsedMs:0;}
};

struct InterLevelState {
    int elapsedMs=0;
    int durationMs=4000; // CONFIRMED: MOV ESI,0xFA0 at 0x0041A9FF.
    std::size_t nextLevel=0;
    float xonixHeight=0.008f;
    float lightScale=1.f;
    float sfxMasterScale=1.f;
    // r115 DIRECT EXE: these are not body Euler angles. 0x41A980 uses the
    // first value as the Y translation of cinematic slots 5/0, and the second
    // as the shared oscillation phase used to build mirrored X/Z rotations.
    float cinematicYOffset=-0.1f;
    float cinematicPhase=0.f;
    float cinematicRotX() const { return std::cos(cinematicPhase)*0.27000001072883606f; }
    float cinematicRotZ() const { return std::sin(cinematicPhase*1.2999999523162842f)*0.21000000834465027f; }
    float rotorPhase=0.f;
    bool speechCueConsumed=false;
    int remainingMs() const{return durationMs>elapsedMs?durationMs-elapsedMs:0;}
    bool legacyParticleWindowActive() const{return remainingMs()>2000;} // 0x41B158 -> 0x417700
    bool legacyOneShotWindowPassed() const{return remainingMs()<3000;} // 0x41AA8D
    bool legacyFadeWindowActive() const{return remainingMs()<2550;} // 0x41AB69 (0x09F6)
};

struct PauseSceneState {
    enum class Stage { Entering, Holding, Leaving };
    Stage stage=Stage::Entering;
    float panelY=-0.30000001192092896f; // 0x41DE96 / 0xBE99999A
    float rotorPhase=0.f;
    bool inputArmed=false;
};

struct AbortConfirmState {
    enum class Stage { Entering, Holding, Leaving };
    Stage stage=Stage::Entering;
    float panelY=-0.25f; // 0x41E584 / 0x43B57C
    bool confirmed=false;
    bool inputArmed=false;
};

struct RecordsState {
    std::size_t mode=0;
    int fadeCounter=0;
    int fadeRate=3;
    bool readyForInput=false;
    bool inputLatched=false;
    // r242 DIRECT EXE: 0x40F0A0 always copies the selected persistent 200-byte
    // block to the working buffer at 0x025457A8. Post-game insertion/editing
    // mutates only that copy until Enter commits it through 0x40F0D0.
    airxonix::LegacyHighScoreBlock workingBlock{};
    bool postGame=false;
    bool nameEntry=false;
    int candidateRow=-1;
    int typedNameLength=0;
    // r247 PortMaster extension: keyboard keeps the original append cursor,
    // while D-Pad editing can select any of the 16 fixed CP1251 cells.
    int nameCursor=0;
    bool dpadNameEditing=false;
    int namePulsePhase=0; // 0x40F3FA: (phase + 5*dt) & 0x7ff
    int headingPhase=0; // r327 0x40F967: (phase + 4*dt) & 0x7ff
    bool dirty=false;
};

struct InformationState {
    // DIRECT EXE 0x410CF0: three sequential pages 0x4101B0, 0x4104A0,
    // 0x414170. Each owns the same +3/-4 transition counter around 0x7C0.
    int page=0;
    int fadeCounter=0;
    int fadeRate=3;
    bool readyForInput=false;
    bool inputLatched=false;
    int promptPhase=0; // r330: row-14 red pulse, +2*dt
};

struct SettingsState {
    enum class PendingTransition { None, Controls, Exit };
    int selected=0;
    // r309 DIRECT EXE 0x413704..0x413769: Settings owns a 0..0x7C0
    // transition counter. Input is accepted only after the +7/ms fade-in clamps.
    int fadeCounter=0;
    int fadeRate=7;
    bool readyForInput=false;
    PendingTransition pendingTransition=PendingTransition::None;
    float speed=800.f;
    float sfx=1000.f;
    float music=800.f;
    bool speech=true;
    bool navLatched=false;
    // r350: 0x413B67..0x413BFA visual selector/camera slide.
    float selectorOffset=0.f;
    bool speechLrLatched=false;
    bool testLrLatched=false;
    int testRepeatMs=0;
    // First six rows are original 0x413670 settings. Native rows 6/7
    // are placed ABOVE row 0 in D-Pad navigation and remain top-right on screen.
    std::array<float,8> brightness{{1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f}};
    std::array<float,8> scale{{1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f}};
    int testInitialLives=3;
    int testInitialTimeSeconds=60;
    std::size_t themeIndex=0;
    // 0x25B7888..0x25B7894: Right, Left, Backward, Forward.
    std::array<int,4> bindings{{0x41,0x5A,0x58,0x43}};
};

struct ControlsRemapState {
    std::array<int,4> temporary{{0x41,0x5A,0x58,0x43}};
    int assigned=0;
    bool awaitingConfirm=false;
    // r313 DIRECT EXE 0x410F2B..0x4112D8: Controls owns the same 0..0x7C0
    // presentation counter pattern as the surrounding menus, but uses +3/ms
    // entering and -4/ms leaving. Input is ignored until the enter fade clamps.
    int fadeCounter=0;
    int fadeRate=3;
    bool readyForInput=false;
    bool exitPending=false;
    bool commitOnExit=false;
    float pulsePhase=0.f;
};


struct ModeSelectState {
    int selected=0;
    bool inputLatched=false;
    int fadeCounter=0;      // 0x411880/0x4118AE: 0..0x7C0 presentation fade.
    int fadeRate=3;         // +3/ms entering, -4/ms leaving.
    bool readyForInput=false;
    bool exitPending=false;
    bool cancelPending=false;
    // r306 DIRECT EXE 0x411B15..0x411C7C: presentation-only animation state.
    // EBP advances by dt*2 and feeds airborne X rotation / crawler Z rotation.
    // The capable background phase advances by dt*.0005 and wraps at 1.0.
    int anglePhase=0;
    float backgroundPhase=0.f;
};

struct MainMenuState {
    int selected=0;
    bool exitPending=false;
    int exitDelayMs=0;
    bool inputLatched=false;
    bool hasPendingDispatch=false;
    LegacyMenuEntryTrace pendingDispatch=kLegacyMainMenuEntries[0];
    // DIRECT EXE r60/r62: persistent arrays at 0x00440D28 (brightness) and
    // 0x00440D40 (scale), both initialized to 1.0. They are approached every
    // menu frame instead of snapping when the selection changes.
    std::array<float,5> brightness{{1.f,1.f,1.f,1.f,1.f}};
    std::array<float,5> scale{{1.f,1.f,1.f,1.f,1.f}};
    // r184 direct caller 0x4131FA..0x413296: visual selector row offset.
    // It approaches selected*(-.003) at dt*.00005 and gates new menu events
    // until it reaches that target. 3-D selector markers use offset-.0015.
    float selectorOffset=0.f;
    std::size_t themeIndex=0;
};

struct LegacyEffectEventState {
    int eraserImpactEvents=0;
    int eraserDebrisParticles=0;
    int eraserSfx23Events=0;
    int pickupSmashEvents=0;
    int pickupSmashDebrisParticles=0;
    int pickupSmashSfx0EEvents=0;
    int airErosionImpactEvents=0;
    int airErosionDebrisHelperCalls=0; // exact 0x4175B0 call count, not particle count
};


struct AuxiliaryEffectState {
    float x=0.f,y=0.5f,z=0.f;
    bool trigger=false;
    bool active() const { return y<0.11f; }
};

struct FinalSceneState {
    int elapsedMs=0;
    float orbitAngle=0.f;
    float orbitRadius=.1f;
    float xonixWorldX=.5f;
    float xonixWorldZ=.5f;
    // 0x41B3/0x41B4/0x41B60C: the local finale Y continues to 0.1,
    // while gameplay-facing 0x257DA4C is capped at 0.04.
    float presentationY=.008f;
    float xonixHeight=.008f;
    float rotorPhase=0.f;
    float rotorRadius=.0037f;
    // r189 0x41B47C..0x41B60C literal state. sceneLight255 starts at
    // 255, eases to 200 at 0.05/ms, then remains there until exit.
    float sceneLight255=255.f;
    float fadeScalar=1.f;
    float exitFadePerMs=0.f;
    float presentationAngle=-0.11999999731779099f;
    float presentationAngleRate=1.9999999494757503e-5f;
    bool inputArmed=false;
    bool exitRequested=false;
    // r142: 0x41B468 seeds the finale cinematic-pair phase from rand()&7.
    // 0x41B94C..0x41BA3B advances it by dt*0.002 and renders slots 5/0
    // with wider mirrored oscillation than the inter-level pair.
    float cinematicPhase=0.f;
    float cinematicRotX() const { return std::cos(cinematicPhase)*0.35999998450279236f; }
    float cinematicRotZ() const { return std::sin(cinematicPhase*1.2999999523162842f)*0.27000001072883606f; }
    int pickupSmashes=0;
};

class Game {
public:
    Game();
    explicit Game(const std::vector<std::uint8_t>& soundInf);
    friend struct GameTestProbe;
    void update(const InputState& input,int dtMs);
    bool wantsQuit() const{return quit_;}
    void showMainMenuOnBoot();
    // debug-only: AIRXONIX_DEBUG_LEVEL="mode,level" selects the start level.
    void debugKill(){ if(phase_==GamePhase::Gameplay)handleDeath(); } // debug-only
    void debugStartSession(){
        std::size_t m=0,l=0;
        if(const char* e=std::getenv("AIRXONIX_DEBUG_LEVEL")){ m=std::size_t(std::strtoul(e,nullptr,10)); if(const char* c=std::strchr(e,','))l=std::size_t(std::strtoul(c+1,nullptr,10)); }
        startNewSession(m); if(l)loadLevel(m,l);
        for(std::size_t i=0;i<20;++i){ try{ const auto& r=database_.level(m,i); std::fprintf(stderr,"AXDBG level %zu,%zu homing=%d eraser=%d crawlers=%d\n",m,i,r.specialHoming,r.specialEraser,r.crawlerCount);}catch(...){break;} }
    }
    bool paused() const{return paused_;}
    float timeScale() const{return difficultyScale_;}
    const Field& field() const{return field_;}
    const Player& player() const{return player_;}
    const Entities& entities() const{return entities_;}
    const Pickups& pickups() const{return pickups_;}
    const SpecialObjects& specialObjects() const{return specialObjects_;}
    int capturePercent() const{return capturePercent_;}
    int hudDisplayTimer() const{return hudDisplayTimer_;}
    int hudDisplayScore() const{return hudDisplayScore_;}
    int hudPulseCounter() const{return hudPulseCounter_;}
    int score() const{return score_;}
    int lives() const{return lives_;}
    int restartCredits() const{return restartCredits_;}
    int timeRemaining() const{return timer_;}
    bool speechEnabled() const{return speechEnabled_;}
    int legacyAudioBasisAngle2() const{return legacyAudioBasisAngle2_;}
    void setSpeechEnabled(bool enabled){speechEnabled_=enabled;}
    float enemySpeedFactor() const{return enemySpeedFactor_;}
    float playerMaxSpeed() const{return playerMaxSpeed_;}
    float brightnessScale() const{return brightnessScale_;}
    float presentationLightScale() const{return levelIntroScene_.active()?levelIntroScene_.lightScale():(phase_==GamePhase::InterLevel?interLevelScene_.lightScale:(phase_==GamePhase::FinalSequence?finalScene_.fadeScalar:1.f));}
    float legacySfxMasterScale() const{const float base=LegacySettingsTrace::audioScale(settings_.sfx);return phase_==GamePhase::InterLevel?base*interLevelScene_.sfxMasterScale:(phase_==GamePhase::FinalSequence?base*finalScene_.fadeScalar:base);}
    float finalePresentationY() const{return finalScene_.presentationY;}
    float legacyMusicMasterScale() const{return LegacySettingsTrace::audioScale(settings_.music);}
    float legacyCameraZoom() const{return legacyCameraZoom_;}
    float legacyBackgroundVPhase() const{return legacyBackgroundVPhase_;}
    bool displayFieldDebris() const{
        if(phase_==GamePhase::FinalSequence)return false; // 0x41BCDD compares bool inputArmed to 2000: unreachable.
        if(phase_==GamePhase::InterLevel)return interLevelScene_.remainingMs()>2000;
        return true;
    }
    int legacyCameraYawOffset() const{return legacyCameraYawOffset_;}
    int displayCameraYawOffset() const{return phase_==GamePhase::Dying?deathScene_.cameraYawOffset:(phase_==GamePhase::GameOver?gameOverScene_.visual.cameraYawOffset:legacyCameraYawOffset_);}
    GamePhase phase() const{return phase_;}
    bool recordNameEntryActive() const{return phase_==GamePhase::Records && records_.nameEntry;}
    bool levelIntroActive() const{return levelIntroScene_.active();}
    bool levelEntryActive() const{return levelEntryScene_.active();}
    const LevelEntryState& levelEntryScene() const{return levelEntryScene_;}
    const LevelIntroState& levelIntroScene() const{return levelIntroScene_;}
    float levelIntroCameraX() const{return levelIntroScene_.cameraX;}
    float startupCameraX() const{return levelEntryActive()?levelEntryScene_.cameraX:levelIntroScene_.cameraX;}
    float startupCameraY() const{return levelEntryActive()?levelEntryScene_.cameraY:levelIntroScene_.cameraY;}
    float startupCameraZ() const{return levelEntryActive()?levelEntryScene_.cameraZ:levelIntroScene_.cameraZ;}
    int startupCameraAngle1() const{return levelEntryActive()?levelEntryScene_.cameraAngle1:levelIntroScene_.cameraAngle1;}
    int startupCameraAngle2() const{return levelEntryActive()?levelEntryScene_.cameraAngle2:levelIntroScene_.cameraAngle2;}
    int startupCameraAngle3() const{return levelEntryActive()?levelEntryScene_.cameraAngle3:levelIntroScene_.cameraAngle3;}
    float levelIntroCameraY() const{return levelIntroScene_.cameraY;}
    float levelIntroCameraZ() const{return levelIntroScene_.cameraZ;}
    int levelIntroCameraAngle1() const{return levelIntroScene_.cameraAngle1;}
    int levelIntroCameraAngle2() const{return levelIntroScene_.cameraAngle2;}
    int levelIntroCameraAngle3() const{return levelIntroScene_.cameraAngle3;}
    const DeathSceneState& deathScene() const{return deathScene_;}
    const DeathOverlayState& deathOverlay() const{return deathOverlay_;}
    const InterLevelState& interLevelScene() const{return interLevelScene_;}
    const GameOverSceneState& gameOverScene() const{return gameOverScene_;}
    const FinalSceneState& finalScene() const{return finalScene_;}
    const MainMenuState& mainMenu() const{return mainMenu_;}
    const ModeSelectState& modeSelect() const{return modeSelect_;}
    const RecordsState& records() const{return records_;}
    const airxonix::LegacyHighScoreBlock& recordsDisplayHighScores() const{return records_.workingBlock;}
    const airxonix::LegacyHighScoreFile& legacyHighScores() const{return highScores_;}
    bool initializeLegacyHighScores(const std::filesystem::path& path);
    bool initializeLegacySettings(const std::filesystem::path& path);
    bool saveLegacySettingsNow() const;
    bool saveLegacyHighScoresNow() const;
    const InformationState& information() const{return information_;}
    // 0x414674/0x414694/0x4146AE: Information page 3 consumes the same
    // process-wide MSVC rand() stream as gameplay. Renderer calls this only
    // from that original presentation path.
    int nextLegacyPresentationRandom(){return rng_.next();}
    std::size_t environmentThemeIndex() const{return environmentThemeIndex_;}
    std::size_t currentMusicTrack() const{return currentMusicTrack_;}
    const SettingsState& settings() const{return settings_;}
    const ControlsRemapState& controlsRemap() const{return controlsRemap_;}
    const AbortConfirmState& abortConfirm() const{return abortConfirm_;}
    const PauseSceneState& pauseScene() const{return pauseScene_;}
    const LegacyEffectEventState& effectEvents() const{return effectEvents_;}
    const std::array<AuxiliaryEffectState,6>& auxiliaryEffects() const{return auxiliaryEffects_;}
    LegacyEffectEventState takeEffectEvents(){ auto out=effectEvents_; effectEvents_={}; return out; }
    std::vector<PickupSmashEvent> takePickupSmashEvents(){ auto out=std::move(pickupSmashEvents_);pickupSmashEvents_.clear();return out; }
    std::vector<DeathAudioEvent> takeDeathAudioEvents(){ auto out=std::move(deathAudioEvents_);deathAudioEvents_.clear();return out; }
    const std::array<DeathBurstParticle,512>& deathBurstParticles() const{return deathBurstParticles_;}
    bool deathBurstActive() const{return deathBurstRemainingMs_>0;}
    const std::array<DeathBurstParticle,LegacyParticles::DeathTriColorCount>& deathTriColorParticles() const{return deathTriColorParticles_;}
    bool deathTriColorInitialized() const{return deathTriColorInitialized_;}
    float displayPlayerWorldX() const{return levelEntryActive()?levelEntryScene_.worldX:(levelIntroActive()?levelIntroScene_.worldX:(phase_==GamePhase::FinalSequence?finalScene_.xonixWorldX:(phase_==GamePhase::Dying?deathScene_.worldX:(phase_==GamePhase::GameOver?gameOverScene_.visual.worldX:player_.worldX()))));}
    float displayPlayerWorldZ() const{return levelEntryActive()?levelEntryScene_.worldZ:(levelIntroActive()?levelIntroScene_.worldZ:(phase_==GamePhase::FinalSequence?finalScene_.xonixWorldZ:(phase_==GamePhase::Dying?deathScene_.worldZ:(phase_==GamePhase::GameOver?gameOverScene_.visual.worldZ:player_.worldZ()))));}
    float displayPlayerHeight() const{return levelEntryActive()?levelEntryScene_.worldY:(levelIntroActive()?levelIntroScene_.worldY:(phase_==GamePhase::FinalSequence?finalScene_.xonixHeight:(phase_==GamePhase::InterLevel?interLevelScene_.xonixHeight:(phase_==GamePhase::Dying?deathScene_.worldY:(phase_==GamePhase::GameOver?gameOverScene_.visual.worldY:player_.visualY())))));}
    float displayPlayerRotorPhase() const{return phase_==GamePhase::FinalSequence?finalScene_.rotorPhase:(phase_==GamePhase::InterLevel?interLevelScene_.rotorPhase:(phase_==GamePhase::Dying?deathScene_.rotorPhase:(phase_==GamePhase::GameOver?gameOverScene_.visual.rotorPhase:playerRotorPhase_)));}
    float displayPlayerPropellerPhase() const{return playerPropellerPhase_;}
    float displayPlayerRotorRadius() const{return phase_==GamePhase::FinalSequence?finalScene_.rotorRadius:(phase_==GamePhase::InterLevel?0.003700000001117587f:(phase_==GamePhase::Dying?deathScene_.rotorRadius:(phase_==GamePhase::GameOver?gameOverScene_.visual.rotorRadius:playerRotorRadius_)));}
    std::size_t modeIndex() const{return mode_;}
    std::size_t levelIndex() const{return level_;}
    std::uint64_t levelLoadSerial() const{return levelLoadSerial_;}
    const LevelDatabase& database() const{return database_;}
private:
    void startNewSession(std::size_t mode);
    void loadLevel(std::size_t mode,std::size_t level);
    void updateScoreAndPercent();
    void updateLegacyEffects(int dtMs);
    void spawnAuxiliaryEffect(int slot,float x,float y,float z);
    void updateAuxiliaryEffects(int dtMs);
    void updateLegacyTimerPressure(int dtMs);
    void applyPickupEffect(PickupEffect effect);
    int updatePickupRuntime(int dtMs,const PickupCollectorState& collector,bool allowPlayerCollection=true);
    void advanceLegacyBackgroundScroll(int dtMs);
    void handleDeath();
    void setPendingDeathDrift(float vx,float vz){pendingDeathDriftX_=vx;pendingDeathDriftZ_=vz;pendingDeathDriftOverride_=true;}
    void spawnDeathBurst(float worldX,float worldY,float worldZ);
    void updateDeathBurst(int dtMs);
    void startDeathOverlay(float x,float y,float z,float r,float g,float b);
    void updateDeathOverlay(int dtMs);
    void initDeathTriColor(float worldX,float worldY,float worldZ);
    void updateDeathTriColor(int dtMs);
    void updateDeathSequence(int dtMs);
    void resolveSpecialPreEntityContacts();
    void consumeSpecialPostUpdateEvents();
    void consumeEntityCollisionAudioEvents();
    void consumeCaptureStartedAudio();
    void queueDeathSpatialPlay(std::size_t logicalId,float x,float y,float z,float scalar);
    void queueDeathSpatialStart(DeathAudioVoiceTag voice,std::size_t logicalId,float x,float y,float z,float scalar);
    void queueDeathSpatialUpdate(DeathAudioVoiceTag voice,float x,float y,float z);
    void queueDeathSpatialStop(DeathAudioVoiceTag voice);
    void queueDeathSimple(std::size_t logicalId);
    void queueDeathMusicFade(float perMs);
    void queueMusicRequest(std::size_t trackId,float transitionScalar);
    void beginGameOverTail();
    void updateGameOverTail(const InputState& input,int dtMs);
    void beginInterLevel(std::size_t nextLevel);
    void updateLevelIntro(int dtMs);
    void updateLevelEntry(int dtMs);
    void updateInterLevel(int dtMs);
    void beginFinalSequence();
    void updateFinalSequence(const InputState& input,int dtMs);
    void beginPresentation(GamePhase phase);
    void selectLevelMusic();
    void enterMainMenu();
    void updatePresentation(const InputState& input);
    void beginPause();
    void updatePause(const InputState& input,int dtMs);
    void beginAbortConfirm();
    void updateAbortConfirm(const InputState& input,int dtMs);
    void updateMainMenu(const InputState& input,int dtMs);
    void enterModeSelect();
    void updateModeSelect(const InputState& input,int dtMs);
    void enterRecords();
    void enterPostGameRecords(std::uint32_t score);
    void updateRecords(const InputState& input,int dtMs);
    void enterInformation();
    void updateInformation(const InputState& input,int dtMs);
    void enterSettings();
    void updateSettings(const InputState& input,int dtMs);
    void enterControlsRemap();
    void updateControlsRemap(const InputState& input,int dtMs);
    void restartCurrentLevel();

    bool quit_=false,paused_=false,pauseLatch_=false;
    bool presentationArmed_=false;
    LegacyRandom rng_{1u};
    LevelDatabase database_;
    Field field_;
    Player player_;
    Entities entities_;
    Pickups pickups_;
    std::vector<PickupSmashEvent> pickupSmashEvents_;
    std::vector<DeathAudioEvent> deathAudioEvents_;
    std::array<DeathBurstParticle,512> deathBurstParticles_{};
    int deathBurstRemainingMs_=0;
    std::array<DeathBurstParticle,LegacyParticles::DeathTriColorCount> deathTriColorParticles_{};
    bool deathTriColorInitialized_=false;
    SpecialObjects specialObjects_;
    std::size_t mode_=0,level_=0;
    std::uint64_t levelLoadSerial_=0; // Port-side observer of native 0x418DD0 level initialization.
    GamePhase phase_=GamePhase::Gameplay;
    LevelIntroState levelIntroScene_{};
    LevelEntryState levelEntryScene_{};
    DeathSceneState deathScene_{};
    DeathOverlayState deathOverlay_{};
    GameOverSceneState gameOverScene_{};
    float pendingDeathDriftX_=0.f,pendingDeathDriftZ_=0.f;
    bool pendingDeathDriftOverride_=false;
    InterLevelState interLevelScene_{};
    FinalSceneState finalScene_{};
    MainMenuState mainMenu_{};
    ModeSelectState modeSelect_{};
    RecordsState records_{};
    airxonix::LegacyHighScoreFile highScores_=airxonix::makeLegacyDefaultHighScores();
    std::filesystem::path highScorePath_{};
    InformationState information_{};
    SettingsState settings_{};
    std::filesystem::path settingsPath_{};
    std::int32_t legacySelectedMode_=0; // gameinf.bin +0x0C / 0x25B7884.
    bool settingsTestVoiceActive_=false;
    ControlsRemapState controlsRemap_{};
    AbortConfirmState abortConfirm_{};
    PauseSceneState pauseScene_{};
    std::array<std::uint32_t,8> menuThemeUsage_{{0,0,0,0,0,0,0,0}};
    std::array<std::uint32_t,12> environmentThemeUsage_{};
    std::array<std::uint32_t,10> musicTrackUsage_{};
    std::size_t environmentThemeIndex_=7;
    std::size_t currentMusicTrack_=0;
    bool levelMusicPending_=true;
    LegacyEffectEventState effectEvents_{};
    std::array<AuxiliaryEffectState,6> auxiliaryEffects_{};
    float difficultyScale_=1.0f,scoreMultiplier_=1.0f;
    // 0x025B7898: gates only spoken/announcer samples in the preserved death CFG.
    // This is the runtime counterpart of the settings item m210 = "РЕЧЬ".
    bool speechEnabled_=true;
    int interLevelSpeechSelector_=0; // 0x0257F540, persistent modulo-4 announcer selector.
    int levelEntrySpeechSelector_=0; // 0x0257F54C, persistent alternating 41/42 startup cue selector.
    int legacyAudioBasisAngle2_=-302; // 0x40A450 callers: Settings=0, session bootstrap=-302.
    std::uint8_t deathSpeechCycleIndex_=0; // 0x0257F548, zero-initialized then (index+1)&3 per spoken death cue.

    // Runtime globals reconstructed from 0x257D9FC / 0x257DADC and nearby.
    float enemySpeedFactor_=1.0f;
    float playerMaxSpeed_=0.03f;
    // 0x257DA94 / 0x257DA98: Xonix rotor phase and radius.
    float playerRotorPhase_=0.0f;
    float playerRotorRadius_=0.0047f;
    // 0x25B5B34 / 0x257DA84: independent central two-blade propeller
    // phase and per-frame angular step. These are distinct from the
    // 0x257DA94/0x257DA98 four-node orbit.
    float playerPropellerPhase_=0.0f;
    float playerPropellerStep_=0.0f;
    // Effect 6: 0x257DAD4 displacement with 0x257DAD8 velocity. Pointer-flow
    // into 0x40C250 proves that it changes camera position: Y uses
    // baseY - 1.1*zoom and Z uses baseZ + zoom. This is the user-identified
    // close-up bonus that deliberately reduces the visible area.
    float legacyCameraZoom_=0.0f;
    // 0x02583614 / 0x41FD50: persistent prepared-background V scroll phase.
    // Initialized by the renderer object constructor and intentionally NOT reset per level/session.
    float legacyBackgroundVPhase_=0.0f;
    float legacyCameraZoomRate_=0.0f;

    // 0x53B684 is not camera distance. 0x40C140/0x40C190 multiply RGB light
    // channels by it, and 0x40C350 packs those values into diffuse color.
    // Effect 7 sets it to zero, producing a blackout/fade that recovers to 1.
    float brightnessScale_=1.0f;
    float brightnessRecoveryRate_=0.00018f;

    float legacyOscAmplitude_=0.0f;
    float legacyOscPhase_=0.0f;
    int legacyCameraYawOffset_=0;

    int lives_=3,score_=0,timer_=61440;
    // r197 runtime counterpart of the r185 0x25B5B60 checkpoint path.
    // A campaign starts with five Backspace restarts; consuming one restores
    // the current level with its entry score/lives checkpoint.
    int restartCredits_=5;
    std::size_t restartCheckpointLevel_=0;
    int restartCheckpointScore_=0;
    int restartCheckpointLives_=3;
    int scoreDecayAccumulatorMs_=0;
    bool lowTimeWarningActive_=false;
    int initialOccupied_=0,lastOccupied_=0,captureDenominator_=1,capturePercent_=0,bonusAccumulator_=0;
    int hudDisplayTimer_=0,hudDisplayScore_=0,hudPulseCounter_=0;
};

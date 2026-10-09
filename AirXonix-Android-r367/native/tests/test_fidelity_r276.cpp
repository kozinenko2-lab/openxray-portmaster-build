#include "game/game.hpp"
#include "game/legacy_level_init_trace.hpp"
#include "game/legacy_session_trace.hpp"
#include "game/legacy_level_integrity_trace.hpp"
#include "render/legacy_hud.hpp"
#include <cassert>
#include <type_traits>

struct GameTestProbe {
    static void setScoreState(Game& g,int score,int timer,int pct,int lives,float scale){
        g.score_=score; g.timer_=timer; g.capturePercent_=pct; g.lives_=lives; g.difficultyScale_=scale;
    }
    static void beginInter(Game& g){ g.beginInterLevel(g.level_+1); }
    static void beginFinal(Game& g){ g.beginFinalSequence(); }
    static void setHud(Game& g,int displayScore,int realScore,int displayTimer,int realTimer){
        g.hudDisplayScore_=displayScore; g.score_=realScore; g.hudDisplayTimer_=displayTimer; g.timer_=realTimer;
        g.phase_=GamePhase::MainMenu;
    }
};

int main(){
    static_assert(kLegacyLevelInit.initialOccupiedAddress==0x0257DA04u);
    static_assert(kLegacyLevelInit.previousOccupiedAddress==0x0257DA08u);
    static_assert(kLegacySessionBootstrap.cameraAngle3Address==0x0257DA70u);
    static_assert(LegacyLevelIntegrityTrace::maskedRuntimeBlockA==0x0257DA0Cu);
    static_assert(LegacyLevelIntegrityTrace::maskedRuntimeBlockB==0x0257DA28u);
    static_assert(std::is_same_v<decltype(DeathSceneState{}.zeroLivesGray),float>);

    Player p; p.reset();
    p.setMaxSpeed(.012f);                    // r265: limit changes, current speed does not clamp immediately.
    assert(p.maxSpeed()==.012f && p.speed()==.03f);
    p.resetForLevel();                       // r266: current speed survives level init.
    assert(p.speed()==.03f && p.maxSpeed()==.03f);

    Game normal;
    GameTestProbe::setScoreState(normal,1000,2000,103,2,1.0f);
    GameTestProbe::beginInter(normal);
    // time 100 + over-capture 3000, sequential truncation.
    assert(normal.score()==4100);

    Game finale;
    GameTestProbe::setScoreState(finale,1000,2000,103,2,1.0f);
    GameTestProbe::beginFinal(finale);
    assert(finale.score()==19100); // +100 time +3000 overcap +10000 lives +5000 completion

    Game hud;
    GameTestProbe::setHud(hud,100,1000,1000,2000);
    hud.update(InputState{},10);
    assert(hud.hudDisplayScore()==420);      // +dt*32
    assert(hud.hudDisplayTimer()==1320);     // symmetric chase
    assert(hud.hudPulseCounter()==10);

    LegacyHudState hs; hs.screen=LegacyHudScreen::Gameplay; hs.capturePercent=103;
    hs.lives=1; hs.timeRemaining=10<<10; hs.pulseCounter=64;
    const auto sprites=LegacyHud::compose(hs);
    // 103% must use three decimal glyphs; low-time/last-life glyphs are tinted.
    int percentDigits=0; bool tinted=false;
    for(const auto& q:sprites){
        if(q.y==460.f && q.x>=121.f && q.x<169.f) ++percentDigits;
        if(q.b<1.f) tinted=true;
    }
    assert(percentDigits>=3);
    assert(tinted);
    return 0;
}

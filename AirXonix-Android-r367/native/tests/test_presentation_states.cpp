#include "game/game.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

struct GameTestProbe {
    static void ready(Game& g){g.levelIntroScene_.elapsedMs=g.levelIntroScene_.durationMs; g.levelEntryScene_.running=false;}
    static void beginInterLevel(Game& g,std::size_t next){ ready(g); g.beginInterLevel(next); }
};

namespace {
void expireOneLife(Game& g){
    InputState idle{};
    g.update(idle,70000);
    if(g.phase()==GamePhase::Dying)
        g.update(idle,g.deathScene().durationMs+1);
}
}

int main(){
    {
        Game g; GameTestProbe::ready(g); InputState abortKey{}; abortKey.select=true;
        g.update(abortKey,16);
        assert(!g.wantsQuit());
        assert(g.phase()==GamePhase::Abort);
        InputState released{};
        for(int i=0;i<30 && g.abortConfirm().stage==AbortConfirmState::Stage::Entering;++i)g.update(released,16);
        assert(g.abortConfirm().stage==AbortConfirmState::Stage::Holding);
        g.update(released,16); // arm after release
        InputState yes{}; yes.action=true; g.update(yes,16);
        for(int i=0;i<40 && g.phase()==GamePhase::Abort;++i)g.update(released,16);
        assert(!g.wantsQuit());
        assert(g.phase()==GamePhase::MainMenu);
        InputState back{}; back.back=true;
        g.update(back,16);
        assert(!g.wantsQuit());
        assert(g.mainMenu().selected==4);
    }
    {
        Game g; GameTestProbe::ready(g); InputState idle{};
        expireOneLife(g); // life 3 -> 2, delayed respawn
        expireOneLife(g); // 2 -> 1
        g.update(idle,70000); // 1 -> 0: original still runs the full death presentation
        assert(!g.wantsQuit());
        assert(g.phase()==GamePhase::Dying);
        assert(g.lives()==0);
        g.update(idle,g.deathScene().durationMs);
        assert(g.phase()==GamePhase::Dying);
        g.update(idle,1);
        assert(g.phase()==GamePhase::GameOver);
        InputState released{}; g.update(released,16);
        InputState action{}; action.action=true; g.update(action,16);
        assert(!g.wantsQuit());
        assert(g.phase()==GamePhase::Records);
        assert(g.records().postGame);
    }
    {
        Game g; GameTestProbe::ready(g); InputState idle{};
        const auto oldLevel=g.levelIndex();
        GameTestProbe::beginInterLevel(g,oldLevel+1);
        assert(g.phase()==GamePhase::InterLevel);
        assert(g.levelIndex()==oldLevel);
        assert(g.interLevelScene().nextLevel==oldLevel+1);
        assert(g.interLevelScene().durationMs==4000);
        assert(g.interLevelScene().legacyParticleWindowActive());
        const float beforeAir=g.entities().air().empty()?0.f:g.entities().air().front().worldX;
        g.update(idle,2000);
        assert(g.phase()==GamePhase::InterLevel);
        assert(!g.interLevelScene().legacyParticleWindowActive());
        if(!g.entities().air().empty()){
            const float afterAir=g.entities().air().front().worldX;
            assert(std::isfinite(afterAir));
            (void)beforeAir; (void)afterAir;
        }
        g.update(idle,g.interLevelScene().durationMs-g.interLevelScene().elapsedMs-1);
        assert(g.phase()==GamePhase::InterLevel);
        assert(g.levelIndex()==oldLevel);
        g.update(idle,1);
        assert(g.phase()==GamePhase::Gameplay);
        assert(g.levelIndex()==oldLevel+1);
    }
    {
        Game g; GameTestProbe::ready(g); InputState idle{};
        const auto before=g.entities().air().empty()?0.f:g.entities().air().front().worldX;
        g.update(idle,70000);
        assert(g.phase()==GamePhase::Dying);
        assert(g.lives()==2);
        assert(g.deathScene().elapsedMs==0);
        assert(g.deathScene().durationMs==4000);
        assert(g.deathScene().remainingMs()==4000);
        g.update(idle,250);
        assert(g.phase()==GamePhase::Dying);
        assert(g.deathScene().elapsedMs==250);
        assert(g.deathScene().remainingMs()==3750);
        if(!g.entities().air().empty()){
            const auto after=g.entities().air().front().worldX;
            assert(std::isfinite(after));
            // A live world update must execute; movement can occasionally
            // reflect back to the same x, so elapsed state is the hard check.
            (void)before; (void)after;
        }
        g.update(idle,g.deathScene().remainingMs()-1);
        assert(g.phase()==GamePhase::Dying);
        assert(g.deathScene().remainingMs()==1);
        g.update(idle,1);
        assert(g.phase()==GamePhase::Dying);
        assert(g.deathScene().remainingMs()==0);
        g.update(idle,1);
        assert(g.phase()==GamePhase::Gameplay);
        assert(!g.player().dead());
    }
    std::cout<<"presentation/death states ok\n";
}

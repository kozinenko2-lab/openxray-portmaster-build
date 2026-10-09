#include "game/game.hpp"
#include "game/legacy_session_trace.hpp"
#include "game/level.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

struct GameTestProbe {
    static void dirtyCampaign(Game& g){
        g.lives_=1; g.score_=7777; g.bonusAccumulator_=999;
        g.level_=3; g.timer_=1234;
    }
    static void start(Game& g,std::size_t mode){ g.startNewSession(mode); }
};

int main(){
    const auto& t=kLegacySessionBootstrap;
    assert(t.routine==0x00418D10u && t.levelInitializer==0x00418DD0u);
    assert(t.caller==0x00424E57u);
    assert(t.initialLives==3 && t.initialScore==0 && t.initialLevel==0);
    assert(t.baseTimeSeconds==60);
    assert(t.modeLevelCounts==LegacyModeResourceTrace::levelCounts);
    assert(std::fabs(t.initialCameraX-.5f)<1e-7f);
    assert(std::fabs(t.initialCameraY-.103f)<1e-7f);
    assert(std::fabs(t.initialCameraZ-.35f)<1e-7f);
    assert(t.initialCameraAngle1==0 && t.initialCameraAngle2==-302);
    assert(t.audioOrientationSetup==0x0040A3D0u);
    assert(t.audioBasisBuild==0x0040A450u);
    assert(t.audioListenerPosition==0x0040A5C0u);
    assert(t.audioBasisAngles[0]==0 && t.audioBasisAngles[1]==-302 && t.audioBasisAngles[2]==0);
    assert(t.pickupVisualReset==0x004156A0u && t.pickupVisualCount==6);
    assert(std::fabs(t.pickupVisualInitial-.5f)<1e-7f);

    Game g;
    GameTestProbe::dirtyCampaign(g);
    GameTestProbe::start(g,2);
    assert(g.modeIndex()==2 && g.levelIndex()==0);
    assert(g.lives()==3 && g.score()==0);
    assert(g.timeRemaining()==(60<<10));
    assert(g.phase()==GamePhase::Gameplay);

    std::cout << "session bootstrap r109 PASS\n";
}

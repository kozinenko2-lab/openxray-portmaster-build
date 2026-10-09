#include "game/game.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

struct GameTestProbe {
    static void prepare(Game& g,int score){
        g.score_=score; g.mode_=0;
        auto& b=g.highScores_.blocks[0];
        for(auto& n:b.names)n.fill('.');
        for(std::size_t i=0;i<10;++i)b.values[i]=1000u-static_cast<std::uint32_t>(i)*100u;
        g.deathScene_={};
        g.deathScene_.elapsedMs=g.deathScene_.durationMs+1;
        g.beginGameOverTail();
    }
    static void arm(Game& g){g.gameOverScene_.inputArmed=true;}
    static void setTimeoutCrossed(Game& g){g.gameOverScene_.elapsedMs=g.gameOverScene_.timeoutMs;}
};

int main(){
    {
        Game g; GameTestProbe::prepare(g,950); GameTestProbe::arm(g);
        InputState in{}; in.action=true;
        g.update(in,0);
        assert(g.phase()==GamePhase::Records);
        assert(g.records().postGame);
        assert(g.records().candidateRow==1);
        assert(g.records().workingBlock.values[1]==950u);
        // Persistent table is still untouched until a valid name is confirmed.
        assert(g.legacyHighScores().blocks[0].values[1]==900u);
        const auto ev=g.takeDeathAudioEvents();
        assert(!ev.empty() && ev.front().logicalId==0x1Fu); // Game Over voice at entry
        assert(ev.back().logicalId==0x16u);                  // dismissal click
    }
    {
        Game g; GameTestProbe::prepare(g,750); GameTestProbe::setTimeoutCrossed(g);
        InputState in{};
        g.update(in,1); // original timeout is strict: > 60000
        assert(g.phase()==GamePhase::Records);
        assert(g.records().postGame && g.records().candidateRow==3);
        const auto ev=g.takeDeathAudioEvents();
        // Timeout path bypasses click 0x16 exactly as 0x41CE7C does.
        for(const auto& e:ev)assert(e.logicalId!=0x16u);
    }
    std::cout << "gameover records r245 PASS\n";
}

#include "game/game.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>

struct GameTestProbe {
    static void prepare(Game& g){
        g.levelIntroScene_.elapsedMs=g.levelIntroScene_.durationMs; g.levelEntryScene_.running=false; g.score_=777u; g.mode_=0; g.capturePercent_=100;
        auto& b=g.highScores_.blocks[0];
        for(auto& n:b.names)n.fill('.');
        for(std::size_t i=0;i<10;++i)b.values[i]=1000u-static_cast<std::uint32_t>(i)*100u;
        g.beginFinalSequence();
    }
};

int main(){
    Game g; GameTestProbe::prepare(g);
    assert(g.phase()==GamePhase::FinalSequence);
    g.update(InputState{},92550);
    assert(g.phase()==GamePhase::Records);
    assert(g.records().postGame);
    // r272 finale scoring runs before the cinematic, so the candidate includes
    // time/over-capture/lives/mode-completion bonuses and is now the top row.
    assert(g.records().candidateRow==0);
    assert(g.records().workingBlock.values[0]>1000u);
    // As with Game Over, the persistent block is untouched before name commit.
    assert(g.legacyHighScores().blocks[0].values[0]==1000u);
    std::cout << "finale records r246 PASS\n";
}

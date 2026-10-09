#include "game/game.hpp"
#include "game/legacy_records_transition.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

struct GameTestProbe {
    static void setBlock(Game& g,const airxonix::LegacyHighScoreBlock& b){g.highScores_.blocks[0]=b;g.mode_=0;}
    static void enterPost(Game& g,std::uint32_t score){g.enterPostGameRecords(score);}
};

static airxonix::LegacyHighScoreBlock ranked(){
    auto b=airxonix::makeLegacyDefaultHighScores().blocks[0];
    for(std::size_t i=0;i<10;++i)b.values[i]=1000u-static_cast<std::uint32_t>(i)*100u;
    return b;
}

static void arm(Game& g){ InputState in{}; g.update(in,700); assert(g.records().readyForInput); }

int main(){
    using namespace LegacyRecordsTransition;
    {
        Game g; const auto original=ranked(); GameTestProbe::setBlock(g,original);
        GameTestProbe::enterPost(g,950u);
        assert(g.phase()==GamePhase::Records);
        assert(g.records().postGame && g.records().nameEntry && g.records().candidateRow==1);
        assert(g.records().workingBlock.values[1]==950u);
        // Working insertion must not touch the persistent block before Enter.
        assert(g.legacyHighScores().blocks[0].values[1]==900u);
        arm(g);
        InputState in{};
        in.legacyPressedCode='A'; g.update(in,0);
        in.legacyPressedCode='B'; g.update(in,0);
        assert(g.records().typedNameLength==2);
        assert(g.records().workingBlock.names[1][0]=='A');
        assert(g.records().workingBlock.names[1][1]=='B');
        in.legacyPressedCode=0x0D; g.update(in,0);
        assert(!g.records().nameEntry && g.records().dirty);
        assert(g.legacyHighScores().blocks[0].values[1]==950u);
        assert(g.legacyHighScores().blocks[0].names[1][0]=='A');
        assert(g.legacyHighScores().blocks[0].names[1][1]=='B');
        // Confirm returns to ordinary Records browsing, it does not exit.
        assert(g.phase()==GamePhase::Records && g.records().fadeRate==FadeInRate);
    }
    {
        Game g; const auto original=ranked(); GameTestProbe::setBlock(g,original);
        GameTestProbe::enterPost(g,950u); arm(g);
        InputState in{}; in.legacyPressedCode=0x1B; g.update(in,0);
        assert(!g.records().nameEntry && !g.records().dirty);
        assert(g.records().fadeRate==FadeOutRate);
        // Cancel never copied the temporary insertion back.
        assert(g.legacyHighScores().blocks[0].values==original.values);
        in={}; g.update(in,500);
        assert(g.phase()==GamePhase::MainMenu);
    }
    {
        Game g; const auto original=ranked(); GameTestProbe::setBlock(g,original);
        GameTestProbe::enterPost(g,100u); // equal to tenth: does not qualify
        assert(!g.records().nameEntry && g.records().candidateRow==-1);
        assert(g.records().workingBlock.values==original.values);
    }
    std::cout << "records name state r242 PASS\n";
}

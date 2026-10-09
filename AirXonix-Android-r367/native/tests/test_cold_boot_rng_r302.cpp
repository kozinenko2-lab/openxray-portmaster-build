#include "game/game.hpp"
#include "core/legacy_random.hpp"
#include <cassert>
#include <iostream>
struct GameTestProbe {
    static std::uint32_t rng(const Game& g){return g.rng_.state();}
    static const auto& menuUsage(const Game& g){return g.menuThemeUsage_;}
    static const auto& envUsage(const Game& g){return g.environmentThemeUsage_;}
};
int main(){
    Game g;
    const auto polluted=GameTestProbe::rng(g);
    assert(polluted!=1u);
    g.showMainMenuOnBoot();
    LegacyRandom ref{1u}; ref.next();
    assert(GameTestProbe::rng(g)==ref.state());
    std::uint32_t menuSum=0;for(auto v:GameTestProbe::menuUsage(g))menuSum+=v;
    assert(menuSum==1u);
    for(auto v:GameTestProbe::envUsage(g))assert(v==0u);
    assert(g.phase()==GamePhase::MainMenu);
    std::cout<<"cold boot rng r302 PASS\n";
}

#include "game/entities.hpp"
#include "game/game.hpp"
#include <cassert>
#include <iostream>

struct EntitiesTestProbe {
    static void erode(Entities& e,Field& f,int x,int y,LegacyRandom& rng){ e.erodeAirImpact(f,x,y,rng); }
};
struct GameTestProbe {
    static void seedEffects(Game& g){
        g.effectEvents_.eraserImpactEvents=2;
        g.effectEvents_.airErosionImpactEvents=3;
        g.effectEvents_.airErosionDebrisHelperCalls=24;
    }
};

int main(){
    Field f;
    for(int y=31;y<=33;++y) for(int x=31;x<=33;++x) f.set(x,y,Field::Safe);
    Entities e;
    LegacyRandom rng(1u);
    EntitiesTestProbe::erode(e,f,32,32,rng);
    assert(e.consumeAirErosionImpactEvents()==1);
    assert(e.consumeAirErosionDebrisHelperCalls()==8);
    assert(e.consumeAirErosionImpactEvents()==0);
    assert(e.consumeAirErosionDebrisHelperCalls()==0);

    Game g;
    GameTestProbe::seedEffects(g);
    auto first=g.takeEffectEvents();
    assert(first.eraserImpactEvents==2);
    assert(first.airErosionImpactEvents==3);
    assert(first.airErosionDebrisHelperCalls==24);
    const auto second=g.takeEffectEvents();
    assert(second.eraserImpactEvents==0);
    assert(second.airErosionImpactEvents==0);
    assert(second.airErosionDebrisHelperCalls==0);
    std::cout << "effect event plumbing ok\n";
}

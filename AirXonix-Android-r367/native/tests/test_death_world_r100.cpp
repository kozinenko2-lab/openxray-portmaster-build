#include "game/legacy_death_world_trace.hpp"
#include "game/entities.hpp"
#include "game/field.hpp"
#include "game/level.hpp"
#include "core/legacy_random.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

struct EntitiesTestProbeR100 {
    static std::vector<GroundEnemy>& ground(Entities& e){ return e.ground_; }
};

int main(){
    const auto& r=kLegacyCrawlerTransitionReset;
    assert(r.routine==0x00416890u && r.recordStride==0x1c);
    assert(r.gridXOffset==0 && r.gridYOffset==4 && r.respawnDelayOffset==0x10);
    assert(std::fabs(r.firstDelay-0.1f)<1e-7f && std::fabs(r.delayStep-0.2f)<1e-7f);

    const auto& w=kLegacyDeathWorldFrame;
    assert(w.begin==0x0041C7D9u && w.endPresentCall==0x0041CE22u);
    assert(w.globalEffectsUpdate==0x004185F0u);
    assert(w.airborneUpdate==0x00416110u && w.crawlerUpdate==0x00416950u);
    assert(w.cameraBuild==0x0040C250u && w.audioListenerPosition==0x0040A5C0u);
    assert(w.xonixDraw==0x004209E0u && w.reflectionFlush==0x0040E960u);
    assert(w.modelPhaseApply==0x00420BB0u);
    assert(w.xonixBodyModelPointer==0x02583738u);
    assert(w.xonixBodyRenderBlob==0x025849D8u);
    assert(w.xonixSecondaryRenderBlob==0x02585A98u);
    assert(w.xonixRotorRenderBlob==0x025B5B28u);
    assert(w.framePresent==0x004060C0u);
    assert(w.reflectionTexture==6 && w.particleTexture==3 && w.shadowTexture==0);

    // Runtime parity for 0x416890: all crawlers are reseeded, while velocity is preserved.
    Entities e;
    auto& g=EntitiesTestProbeR100::ground(e);
    g.resize(3);
    for(std::size_t i=0;i<g.size();++i){
        g[i].x=7.f+i; g[i].y=9.f+i; g[i].vx=0.25f+float(i); g[i].vy=-0.5f-float(i);
        g[i].worldX=0.71f+0.01f*float(i); g[i].worldZ=0.81f+0.01f*float(i);
        g[i].respawnDelay=9.f;
    }
    e.resetCrawlerTransition();
    for(std::size_t i=0;i<g.size();++i){
        assert(g[i].x==32.f && g[i].y==65.f);
        assert(std::fabs(g[i].respawnDelay-(0.1f+0.2f*float(i)))<1e-6f);
        assert(g[i].vx==0.25f+float(i));
        assert(g[i].vy==-0.5f-float(i));
        assert(std::fabs(g[i].worldX-(0.71f+0.01f*float(i)))<1e-6f);
        assert(std::fabs(g[i].worldZ-(0.81f+0.01f*float(i)))<1e-6f);
    }
    std::cout << "death world r100 PASS\n";
}

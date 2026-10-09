#include "render/legacy_models.hpp"
#include <cassert>
#include <cmath>
#include <cstring>

int main(){
    const auto& t=LegacyModels::RecordsCrawlerDecoration;
    assert(t.builderRoutine==0x00420C10u);
    assert(t.masterStore==0x00420F1Du);
    assert(t.masterObject==0x02585A5Cu);
    assert(t.preparedChild==0x0257F5B8u);
    assert(t.directSubmitCalls.front()==0x0040FD22u);
    assert(t.directSubmitCalls.back()==0x0040FDADu);
    assert(t.reflectionQueueCalls.front()==0x0040FDD1u);
    assert(t.reflectionQueueCalls.back()==0x0040FE9Fu);
    assert(t.alphaEnableCall==0x0040FF29u);
    assert(t.texture6SelectCall==0x0040FF30u);
    assert(t.reflectionFlushCall==0x0040FF37u);
    assert(t.reflectionTextureHandle==6);
    assert(std::strcmp(t.reflectionTextureResource,"1111")==0);
    assert(t.airEnemyBuilder==0x00421540u);
    assert(t.finalAirEnemySubtypes[0]==0 && t.finalAirEnemySubtypes[1]==2);
    assert(t.finalAirEnemyMasters[0]==0x0257F574u);
    assert(t.finalAirEnemyMasters[1]==0x0257F57Cu);
    assert(t.finalAirEnemySubmitCalls[0]==0x0040FEF6u);
    assert(t.finalAirEnemySubmitCalls[1]==0x0040FF24u);
    assert(std::fabs(t.laneRatePerMs-0.00004f)<1e-9f);
    assert(std::fabs(t.laneWrapSpan-0.16f)<1e-6f);
    assert(std::fabs(t.initialLaneY[0]-0.08f)<1e-6f);
    assert(std::fabs(t.initialLaneY[1]-0.02666f)<1e-6f);
    assert(std::fabs(t.initialLaneY[5]+0.05333f)<1e-6f);
    return 0;
}

#include "game/level.hpp"
#include <cassert>
#include <iostream>

int main(){
    static_assert(LegacyModeResourceTrace::physicalRecordCapacity==256u);
    static_assert(LegacyModeResourceTrace::normalLevelRecords==82u);
    static_assert(LegacyModeResourceTrace::unreachableTailRecordRegions==174u);
    static_assert(LegacyModeResourceTrace::soleLevelRecordBaseXref==0x00424EE9u);
    static_assert(LegacyModeResourceTrace::maxReachableRecordIndex==81u);
    unsigned maxIndex=0;
    for(std::size_t i=0;i<LegacyModeResourceTrace::modeCount;++i){
        const unsigned last=LegacyModeResourceTrace::levelStarts[i]+LegacyModeResourceTrace::levelCounts[i]-1u;
        if(last>maxIndex) maxIndex=last;
    }
    assert(maxIndex==81u);
    std::cout << "r157 level-record tail is unreachable payload, not hidden levels\n";
}

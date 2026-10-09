#include "render/legacy_models.hpp"
#include <cassert>
#include <iostream>
int main(){
    const auto& c=LegacyModels::CinematicPreparedSet;
    assert(c.builderRoutine==0x004223E0u);
    assert(c.preparedStore==0x00422584u);
    assert(c.preparedBase==0x0257F5CCu);
    assert(c.count==6 && c.pointerStride==4);
    for(int i=0;i<c.count;++i) assert(c.prepared[static_cast<std::size_t>(i)]==c.preparedBase+static_cast<std::uint32_t>(i*4));
    assert(c.prepared[1]==0x0257F5D0u);
    assert(c.confirmedUses[5]==0x0041DAF8u);
    assert(c.confirmedUses[6]==0x0041DD9Fu);
    std::cout<<"0x4223E0 six cinematic prepared handles classified\n";
}

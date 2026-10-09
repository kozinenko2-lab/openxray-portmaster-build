#include "render/legacy_level_intro_backend_trace.hpp"
#include <cassert>
#include <iostream>
int main(){
    const auto& t=kLegacyLevelIntroBackendTrace;
    assert(t.capabilityGlobal==0x0045000Cu);
    assert(t.capabilitySet==0x00408440u);
    assert(t.capablePath==0x0041D4D0u && t.fallbackPath==0x0041DBC0u);
    assert(t.requiredBit==0x02u);
    assert(t.gles2UsesCapablePath());
    std::cout << "level intro backend r293 PASS\n";
}

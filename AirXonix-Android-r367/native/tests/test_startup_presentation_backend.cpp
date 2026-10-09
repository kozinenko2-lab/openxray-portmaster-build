#include "render/legacy_models.hpp"
#include <cassert>
int main(){
    const auto& s=LegacyModels::StartupPresentationBackend;
    assert(s.sessionRoutine==0x004195D0u);
    assert(s.dispatchCall==0x00419602u);
    assert(s.dispatcherRoutine==0x0041CEA0u);
    assert(s.capabilityFlag==0x0045000Cu);
    assert(s.capablePathCall==0x0041CEBCu);
    assert(s.capablePath==0x0041D4D0u);
    assert(s.fallbackPathCall==0x0041CEC3u);
    assert(s.fallbackPath==0x0041DBC0u);
    assert(s.capableTexture7Call==0x0041DAC6u);
    assert(s.capableTexture4Call==0x0041DB0Fu);
    assert(s.fallbackTexture3Call==0x0041DD82u);
    assert(s.renderStateCapabilityGuard==0x00405A60u);
    return 0;
}

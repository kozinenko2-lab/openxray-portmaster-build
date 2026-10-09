#include <cassert>
#include <cstring>
#include "render/legacy_theme.hpp"

int main(){
    const auto& tr=kLegacyEnvironmentThemeTrace;
    assert(tr.tableAddress==0x004418B0u);
    assert(tr.selectRoutine==0x00422FC0u);
    assert(tr.gameplayConstructor==0x00423350u);
    assert(tr.uploadRoutine==0x004058F0u);
    // Constructor mapping proved by 0x42335F/0x423364/0x42336A and the first
    // 0x4058F0 records: +4 table field -> logical slot 0, +0 -> slot 1.
    assert(tr.runtimeSlot0Global==0x025B5B48u);
    assert(tr.runtimeSlot1Global==0x025B7870u);
    assert(tr.runtimeSlot2Global==0x025B5B4Cu);
    const auto& t=kLegacyEnvironmentThemes[kLegacyReferenceTheme];
    assert(std::strcmp(t.slot0,"0212")==0);
    assert(std::strcmp(t.slot1,"VOL0")==0);
    assert(std::strcmp(t.slot2,"0049")==0);
    return 0;
}

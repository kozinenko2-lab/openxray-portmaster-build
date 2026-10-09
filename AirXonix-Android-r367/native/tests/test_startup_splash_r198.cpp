#include "render/legacy_main_menu_decor.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

static bool nearf(float a,float b,float eps=1e-6f){ return std::fabs(a-b)<=eps; }

int main(){
    using T=LegacyMainMenuDecorationTrace;
    static_assert(T::startupBlackPreRollMs==0x320);
    static_assert(T::titleHoldMs==0x5dc);
    static_assert(!T::resetTitleStateOnMainMenuReentry);
    assert(nearf(T::titleDropInitial,0.06f));
    assert(nearf(T::entryInitial,-0.5f));

    T::TitleState title{};
    float entry=T::entryInitial;

    // While reveal is zero, 0x411D60 must leave the M1 entry at -0.5.
    for(int i=0;i<12;++i){
        const auto before=title;
        entry=T::advanceEntryPhase(entry,before,100);
        T::advanceTitle(title,100);
        assert(nearf(entry,T::entryInitial));
    }
    assert(nearf(title.drop,0.f));
    assert(title.reveal==0.f);

    // 0x411EF0 starts reveal only when wait > 1500, not at ==1500.
    title.waitMs=1400; title.reveal=0.f; title.drop=0.f; entry=T::entryInitial;
    auto before=title;
    entry=T::advanceEntryPhase(entry,before,100);
    T::advanceTitle(title,100);
    assert(title.waitMs==1500);
    assert(nearf(title.reveal,0.f));
    assert(nearf(entry,T::entryInitial));

    before=title;
    entry=T::advanceEntryPhase(entry,before,1);
    T::advanceTitle(title,1);
    assert(title.waitMs==1501);
    assert(title.reveal>0.f);
    // Same frame is still logo-only because 0x411D60 preceded 0x411EF0.
    assert(nearf(entry,T::entryInitial));
    assert(T::introOnly(entry));

    // The following frame is the first frame allowed to move M1 into view.
    before=title;
    entry=T::advanceEntryPhase(entry,before,1);
    assert(entry>T::entryInitial);
    assert(!T::introOnly(entry));

    // Process-lifetime state must remain advanced on a conceptual M1 re-entry.
    const auto persisted=title;
    assert(persisted.reveal>0.f);
    assert(!T::resetTitleStateOnMainMenuReentry);

    std::cout << "r198 startup splash timing/order contract verified\
";
    return 0;
}

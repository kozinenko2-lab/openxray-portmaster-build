#include "game/field.hpp"
#include <cassert>
#include <iostream>

struct FieldTestProbe {
    static void setMarkerPhase(Field& f,std::uint8_t m,float phase){ f.markerTimer_[m]=phase; }
};

int main(){
    Field f;
    LevelRecord level{};
    f.build(level);
    for(int y=1;y<63;++y)f.set(32,y,Field::Safe);
    const auto marker=f.beginCapture({GridSeed{16,31}});
    assert(marker!=0);
    assert(f.markerCellCount(marker)>0);

    // 0x418637: equality sets C3, so test ah,0x41 jumps over conversion.
    FieldTestProbe::setMarkerPhase(f,marker,0.00800000037997961f);
    f.updateCaptureAnimations(0);
    assert(f.markerActive(marker));
    assert(f.markerCellCount(marker)>0);

    // Any positive increment from the exact threshold makes phase greater and
    // the EXE converts every byte carrying this marker to SAFE (0x20).
    f.updateCaptureAnimations(1);
    assert(!f.markerActive(marker));
    assert(f.markerCellCount(marker)==0);
    assert(f.at(48,31)==Field::Safe);

    std::cout << "capture marker threshold r254 ok\n";
    return 0;
}

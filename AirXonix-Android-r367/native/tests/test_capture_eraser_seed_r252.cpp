#include "game/special_objects.hpp"
#include "game/field.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

struct SpecialObjectsTestProbe {
    static EraserSpecial& eraser(SpecialObjects& s){ return s.eraser_; }
};

int main(){
    SpecialObjects special;
    GridSeed seed{};

    // 0x4184CD: zero 0x254A290 skips the additional capture seed.
    auto& e=SpecialObjectsTestProbe::eraser(special);
    e.active=true;
    e.speed=0.f;
    e.worldX=.5f;
    e.worldZ=.5f;
    assert(!special.eraserCaptureSeed(seed));

    // r270 correction: 0x4184E0..0x418510 calls 0x43129C, truncating toward zero.
    e.speed=.000002f;
    assert(special.eraserCaptureSeed(seed));
    assert(seed.x==31 && seed.y==31);

    e.worldX=.45f;
    e.worldZ=.55f;
    assert(special.eraserCaptureSeed(seed));
    const int expectedX=static_cast<int>((.45f-.4015600085f)*320.f);
    const int expectedY=static_cast<int>((.55f-.4015600085f)*320.f);
    assert(seed.x==expectedX && seed.y==expectedY);

    // Integration-level capture semantics: split the empty interior into two
    // components with a SAFE wall. The eraser-side component must be restored
    // to Empty; the other side remains the new marker and is captured later.
    Field f;
    LevelRecord level{};
    f.build(level);
    for(int y=1;y<63;++y)f.set(32,y,Field::Safe);
    GridSeed left{16,31};
    const auto marker=f.beginCapture({left});
    assert(marker!=0);
    assert(f.at(16,31)==Field::Empty);
    assert(f.at(48,31)==marker);

    std::cout << "capture eraser seed r252 ok\n";
    return 0;
}

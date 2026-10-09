#include "game/level.hpp"
#include <cassert>
#include <iostream>
int main(){
    auto data=makeCleanroomSoundInf();
    const char names[5][12]={{'A','L','P','H','A',0},{'B','E','T','A',0},{'G','A','M','M','A',0},{'D','E','L','T','A',0},{'O','M','E','G','A',0}};
    for(std::size_t m=0;m<5;++m)
        for(std::size_t i=0;i<12;++i)data[LegacyModeResourceTrace::modeNamesOffset+m*12+i]=static_cast<unsigned char>(names[m][i]);
    LevelDatabase db(data);
    assert(db.modes().size()==5);
    assert(db.modes()[0].name=="ALPHA");
    assert(db.modes()[1].name=="BETA");
    assert(db.modes()[4].name=="OMEGA");
    std::cout<<"soundinf names r324 PASS\n";
}

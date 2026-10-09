#include "game/level.hpp"
#include <cassert>
#include <iostream>
int main(){
    auto data=makeCleanroomSoundInf();
    const unsigned char custom[12]={'M','O','D','E','-','X',0,0,0,0,0,0};
    for(std::size_t i=0;i<12;++i)data[LegacyModeResourceTrace::modeNamesOffset+i]=custom[i];
    LevelDatabase db(data);
    assert(db.modes()[0].name=="MODE-X");
    std::string field(11,' ');
    const auto& name=db.modes()[0].name; const std::size_t n=std::min<std::size_t>(11,name.size());
    field.replace((11-n)/2,n,name.data(),n);
    assert(field=="  MODE-X   ");
    std::cout<<"records soundinf name r325 PASS\n";
}

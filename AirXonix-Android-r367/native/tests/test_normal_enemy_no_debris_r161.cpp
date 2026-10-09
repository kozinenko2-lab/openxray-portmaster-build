#include <cassert>
#include <fstream>
#include <sstream>
#include <string>
int main(){
    std::ifstream f("../src/game/entities.cpp");
    if(!f) f.open("src/game/entities.cpp");
    std::stringstream ss; ss<<f.rdbuf(); const std::string s=ss.str();
    // r202 supersedes the old r161 interpretation: all four airborne subtypes
    // call 0x415FA0; subtype only selects old-vs-candidate erosion centre.
    assert(s.find("erodeAirImpact(field,e.subtype>1?cx:ox,e.subtype>1?cy:oy,rng)")!=std::string::npos);
}

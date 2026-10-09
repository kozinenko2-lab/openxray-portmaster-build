#include "render/legacy_mesh_builder.hpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

int main(){
    LegacyMeshBuilder b;
    b.addBeveledPrism(-0.02f,0.0004f,-0.02f,0.04f,0.04f,0.001f,0.f,1.f,0.f,1.f);
    const auto m=b.take();
    if(m.vertices.size()!=32 || m.faces.size()!=14) return 1;
    const std::array<std::array<std::uint32_t,4>,14> expected{{
        {{7,0,1,2}}, {{7,2,3,6}}, {{6,3,4,5}},
        {{10,9,8,15}}, {{14,11,10,15}}, {{13,12,11,14}},
        {{17,16,24,25}}, {{18,17,25,26}}, {{19,18,26,27}}, {{20,19,27,28}},
        {{21,20,28,29}}, {{22,21,29,30}}, {{23,22,30,31}}, {{16,23,31,24}}
    }};
    for(std::size_t i=0;i<expected.size();++i){
        if(m.faces[i].index.size()!=4) return 2;
        for(std::size_t j=0;j<4;++j){
            if(m.faces[i].index[j]!=expected[i][j]){
                std::cerr<<"face "<<i<<" index "<<j<<" got "<<m.faces[i].index[j]
                         <<" expected "<<expected[i][j]<<"\n";
                return 3;
            }
        }
    }
    return 0;
}

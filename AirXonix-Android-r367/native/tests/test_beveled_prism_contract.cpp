#include "render/legacy_mesh_builder.hpp"
#include <cmath>
#include <iostream>

int main(){
    LegacyMeshBuilder b;
    b.addBeveledPrism(-0.02f,0.0004f,-0.02f,0.04f,0.04f,0.001f,0.f,1.f,0.f,1.f);
    const auto m=b.take();
    if(m.vertices.size()!=32){std::cerr<<"0x404620 vertex contract: "<<m.vertices.size()<<" != 32\n";return 1;}
    if(m.faces.size()!=14){std::cerr<<"0x404620 face contract: "<<m.faces.size()<<" != 14\n";return 2;}
    for(const auto& f:m.faces)if(f.index.size()!=4){std::cerr<<"non-quad face\n";return 3;}
    int up=0,down=0,horizontal=0;
    for(const auto& v:m.vertices){
        if(std::fabs(v.ny-1.f)<1e-6f)++up;
        else if(std::fabs(v.ny+1.f)<1e-6f)++down;
        else if(std::fabs(v.ny)<1e-6f && std::fabs(std::sqrt(v.nx*v.nx+v.nz*v.nz)-1.f)<1e-4f)++horizontal;
    }
    if(up!=8||down!=8||horizontal!=16){
        std::cerr<<"normal groups "<<up<<"/"<<down<<"/"<<horizontal<<" expected 8/8/16\n";return 4;
    }
    return 0;
}

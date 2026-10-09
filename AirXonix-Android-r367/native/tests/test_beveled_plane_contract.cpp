#include "render/legacy_mesh_builder.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

int main(){
    LegacyMeshBuilder b;
    b.addBeveledPlane(-.02f,0.f,-.02f,.04f,.04f,.001f,0.f,1.f,0.f,1.f);
    const auto& m=b.mesh();
    assert(m.vertices.size()==8);
    assert(m.faces.size()==3);
    for(const auto& f:m.faces) assert(f.index.size()==4);
    for(const auto& v:m.vertices){
        assert(std::fabs(v.nx)<1e-7f);
        assert(std::fabs(v.ny-1.f)<1e-7f);
        assert(std::fabs(v.nz)<1e-7f);
    }
    std::cout << "0x4041A0 beveled-plane contract ok\n";
}

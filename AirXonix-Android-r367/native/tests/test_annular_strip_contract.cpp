#include "render/legacy_mesh_builder.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

int main(){
    constexpr int segments=8;
    LegacyMeshBuilder a,b;
    a.addAnnularStrip(0.f,0.f,.1f,.2f,0.f,segments,0.f,1.0f,0.f,1.f,0.f,1.f,false);
    b.addAnnularStrip(0.f,0.f,.1f,.2f,0.f,segments,0.f,1.0f,0.f,1.f,0.f,1.f,true);
    const auto& ma=a.mesh(); const auto& mb=b.mesh();
    assert(ma.vertices.size()==std::size_t(2*(segments+1)));
    assert(mb.vertices.size()==ma.vertices.size());
    assert(ma.faces.size()==std::size_t(segments));
    assert(mb.faces.size()==ma.faces.size());
    for(const auto& f:ma.faces) assert(f.index.size()==4);
    for(const auto& f:mb.faces) assert(f.index.size()==4);
    // Surviving reconstruction produces the same visible strip coordinates;
    // handedness is carried by opposite quad winding.
    for(std::size_t i=0;i<ma.vertices.size();++i){
        assert(std::fabs(ma.vertices[i].x-mb.vertices[i].x)<1e-6f);
        assert(std::fabs(ma.vertices[i].z-mb.vertices[i].z)<1e-6f);
    }
    assert(ma.faces[0].index[0]==0 && ma.faces[0].index[1]==1 && ma.faces[0].index[2]==3 && ma.faces[0].index[3]==2);
    assert(mb.faces[0].index[0]==2 && mb.faces[0].index[1]==3 && mb.faces[0].index[2]==1 && mb.faces[0].index[3]==0);
    std::cout << "0x403400/0x4036E0 annular handedness ok\n";
}

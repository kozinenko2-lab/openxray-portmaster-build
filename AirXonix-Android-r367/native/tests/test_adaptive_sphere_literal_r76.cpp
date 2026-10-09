#include "render/legacy_mesh_builder.hpp"
#include <cassert>
#include <cstddef>

static void check(int n,std::size_t vertices,std::size_t triangles){
    LegacyMeshBuilder b;
    b.addAdaptiveSphere(n,1.f,0.f,1.f,0.f,1.f);
    const auto& m=b.mesh();
    assert(m.vertices.size()==vertices);
    assert(m.faces.size()==triangles);
    for(const auto& f:m.faces) assert(f.index.size()==3u);
    // r74 scratch/dead entry.
    assert(m.vertices[0].x==0.f && m.vertices[0].y==0.f && m.vertices[0].z==0.f);
    assert(m.vertices[0].nx==1.f && m.vertices[0].ny==0.f && m.vertices[0].nz==0.f);
    for(const auto& f:m.faces) for(auto i:f.index) assert(i!=0u);
}
int main(){
    check(8,22,32);
    check(12,44,72);
    check(16,74,128);
    return 0;
}

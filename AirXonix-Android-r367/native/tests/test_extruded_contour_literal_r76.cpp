#include "render/legacy_mesh_builder.hpp"
#include <cassert>
#include <cmath>
#include <vector>
int main(){
    std::vector<LegacyProfilePoint> p;
    // 18 points with explicit duplicate seam endpoint, matching the heart contract.
    for(int i=0;i<17;++i) p.push_back({float(i),float((i*3)%7)});
    p.push_back(p.front());
    LegacyMeshBuilder b;
    b.addExtrudedContour(p,0.f,0.f,-.1f,.1f,.25f,.75f,1.f);
    const auto& m=b.mesh();
    assert(m.vertices.size()==72u); // 4P
    assert(m.faces.size()==33u);    // 17 side + 8 front + 8 back
    for(const auto& f:m.faces) assert(f.index.size()==4u);
    // First and last literal side faces.
    assert((m.faces[0].index==std::vector<std::uint32_t>{0,1,3,2}));
    assert((m.faces[16].index==std::vector<std::uint32_t>{32,33,35,34}));
    // DIRECT EXE 0x40283D..0x40294C: cap normals carry a radial XY component
    // of 0.15; front NZ is the literal -0.8 while back NZ is +1.0.
    const auto& front0=m.vertices[37];
    const auto& back0=m.vertices[55];
    assert(std::fabs(front0.nx-(0.15f/std::sqrt(10.f)))<1e-6f);
    assert(std::fabs(front0.ny-(0.45f/std::sqrt(10.f)))<1e-6f);
    assert(std::fabs(front0.nz+0.8f)<1e-6f);
    assert(std::fabs(back0.nx-(0.15f/std::sqrt(10.f)))<1e-6f);
    assert(std::fabs(back0.ny-(0.45f/std::sqrt(10.f)))<1e-6f);
    assert(std::fabs(back0.nz-1.0f)<1e-6f);
    // First front/back cap quads; front group starts at 36, back at 54.
    assert((m.faces[17].index==std::vector<std::uint32_t>{36,37,38,39}));
    assert((m.faces[18].index==std::vector<std::uint32_t>{57,56,55,54}));
    // Last cap quads preserve the degenerate seam endpoint structurally.
    assert((m.faces[31].index==std::vector<std::uint32_t>{36,51,52,53}));
    assert((m.faces[32].index==std::vector<std::uint32_t>{71,70,69,54}));
    return 0;
}

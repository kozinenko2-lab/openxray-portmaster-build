#include "render/legacy_model_factory.hpp"
#include "render/legacy_models.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

int main(){
    const LegacyMesh m=LegacyModelFactory::buildAirEnemyShadow();
    const auto& spec=LegacyModels::AirEnemyShadow;
    assert(m.vertices.size()==14u);
    assert(m.faces.size()==6u);
    for(const auto& v:m.vertices){
        assert(std::fabs(v.y-spec.builderY)<1e-8f);
        const float r=std::sqrt(v.x*v.x+v.z*v.z);
        assert(std::fabs(r-spec.radius)<1e-6f);
    }
    for(std::size_t i=0;i<m.faces.size();++i){
        assert(m.faces[i].index.size()==4u);
        for(std::size_t j=0;j<4;++j)
            assert(m.faces[i].index[j]==static_cast<std::uint32_t>(spec.literalFaces[i][j]));
    }
    assert(spec.sourceVertexCount==14);
    assert(spec.faceCount==6 && spec.faceArity==4);
    assert(spec.transformedIndexStride==44u);
    assert(spec.faceStreamBegin==0x00422D3Du);
    assert(spec.faceStreamEnd==0x00422D6Au);
    assert(spec.confirmedTextureHandle==0);
    assert(spec.builderRoutine==0x00422D20u);
    assert(spec.drawRoutine==0x00422DE0u);
    assert(spec.inputVertexStride==24u);
    assert(spec.textureUOffset==12u && spec.textureVOffset==16u && spec.lightScalarOffset==20u);
    assert(spec.transformedSubmitRoutine==0x0040C350u);
    assert(std::fabs(spec.builderY-0.0009f)<1e-8f);
    assert(std::fabs(spec.uvWorldBias-0.4f)<1e-6f);
    assert(std::fabs(spec.uvWorldScale-20.f)<1e-6f);
    assert(std::fabs(spec.initialLightScalar-0.34455845f)<1e-6f);
    std::cout<<"0x422D20 literal 14-vertex/6-quad XYZ+UV+light shadow contract ok\n";
}

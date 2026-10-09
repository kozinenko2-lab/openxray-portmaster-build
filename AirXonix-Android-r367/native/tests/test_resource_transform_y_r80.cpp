#include "core/paths.hpp"
#include "render/legacy_models.hpp"
#include "render/legacy_transform.hpp"
#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>

static bool near(float a,float b,float eps=1e-6f){return std::fabs(a-b)<=eps;}

int main(){
    namespace fs=std::filesystem;
    const fs::path root=fs::temp_directory_path()/"airxonix-r80-path-test";
    std::error_code ec; fs::remove_all(root,ec);
    fs::create_directories(root/"assets"/"MUSIC");
    { std::ofstream f(root/"assets"/"AirXonix.wrp.exe",std::ios::binary); f << "MZ"; }
    const auto paths=makeGamePathsFromRoot(root);
    assert(fs::path(paths.originalExe)==root/"assets"/"AirXonix.wrp.exe");
    assert(fs::path(paths.originalMusic)==root/"assets"/"MUSIC");

    // Exact 0x401490 application convention: columns, not rows.
    LegacyTransform::Matrix34 m{};
    m.m[0]=1.f; m.m[1]=2.f; m.m[2]=3.f;
    m.m[3]=4.f; m.m[4]=5.f; m.m[5]=6.f;
    m.m[6]=7.f; m.m[7]=8.f; m.m[8]=9.f;
    m.scalar=1.f;
    const auto p=LegacyTransform::transformPoint(m,10.f,20.f,30.f,1.f,2.f,3.f);
    assert(near(p.x,10.f*1.f+20.f*4.f+30.f*7.f+1.f));
    assert(near(p.y,10.f*2.f+20.f*5.f+30.f*8.f+2.f));
    assert(near(p.z,10.f*3.f+20.f*6.f+30.f*9.f+3.f));

    assert(near(LegacyModels::SafeFieldTopY,0.008f));
    assert(near(LegacyModels::NonSafeFieldY,0.0f));
    assert(near(LegacyModels::FieldBoundaryLowY,0.0005f));
    assert(near(LegacyModels::AirEnemySubmitY,0.005f));
    assert(near(LegacyModels::GroundEnemySubmitYOffset,0.013f));
    assert(near(LegacyModels::PickupSubmitYOffset,0.007f));
    fs::remove_all(root,ec);
    std::cout << "r80 resource path + transform + submit Y regression OK\n";
}

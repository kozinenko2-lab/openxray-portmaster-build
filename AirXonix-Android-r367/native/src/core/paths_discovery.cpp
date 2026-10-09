#include "paths.hpp"
#include <filesystem>
#include <initializer_list>

bool fileExists(const std::string& path) {
    std::error_code ec;
    return !path.empty() && std::filesystem::is_regular_file(path, ec);
}

std::string firstExistingFile(std::initializer_list<std::string> candidates){
    for(const auto& p:candidates)if(fileExists(p))return p;
    return {};
}

GamePaths makeGamePathsFromRoot(const std::filesystem::path& inputRoot) {
    const std::filesystem::path root=inputRoot.lexically_normal();
    GamePaths p;
    p.root = root.string();
    p.assets = (root / "assets").string();
    p.textures = (root / "assets" / "textures").string();
    p.raw = (root / "assets" / "raw").string();
    p.music = (root / "assets" / "music").string();
    p.saves = (root / "saves").string();
    p.originalExe=firstExistingFile({
        (root/"AirXonix.wrp.exe").string(),
        (root/"original"/"AirXonix.wrp.exe").string(),
        (root/"AirXonix"/"AirXonix.wrp.exe").string(),
        (root/"game"/"AirXonix.wrp.exe").string(),
        // r80: this is the natural PortMaster layout used by hardware testers.
        (root/"assets"/"AirXonix.wrp.exe").string(),
        (root/"assets"/"original"/"AirXonix.wrp.exe").string()
    });
    if(!p.originalExe.empty()){
        const std::filesystem::path originalRoot=std::filesystem::path(p.originalExe).parent_path();
        for(const auto& d:{originalRoot/"MUSIC",originalRoot/"music",root/"assets"/"MUSIC",root/"MUSIC",root/"music"}){
            std::error_code ec;
            if(std::filesystem::is_directory(d,ec)){p.originalMusic=d.string();break;}
        }
    }
    std::error_code ec;std::filesystem::create_directories(p.saves, ec);
    return p;
}

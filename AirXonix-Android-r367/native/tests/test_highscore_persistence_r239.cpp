#include "game/game.hpp"
#include "core/legacy_highscore.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

int main(){
    const auto root=std::filesystem::temp_directory_path()/"airxonix_hscore_r239";
    std::error_code ec;std::filesystem::remove_all(root,ec);std::filesystem::create_directories(root,ec);
    const auto path=root/"hscore.bin";

    // Missing file: exact EXE behaviour creates all eight default blocks.
    Game a;
    assert(a.initializeLegacyHighScores(path));
    assert(std::filesystem::file_size(path)==airxonix::LegacyHighScoreTrace::fileSize);
    for(const auto& block:a.legacyHighScores().blocks){
        for(const auto& name:block.names)for(char c:name)assert(c=='.');
        for(auto v:block.values)assert(v==0u);
    }

    // A short file is a failed strict ReadFile and is replaced by 0x640 bytes.
    {std::ofstream f(path,std::ios::binary|std::ios::trunc);f.write("short",5);}
    Game b;
    assert(b.initializeLegacyHighScores(path));
    assert(std::filesystem::file_size(path)==airxonix::LegacyHighScoreTrace::fileSize);
    assert(b.legacyHighScores().blocks[0].values[0]==0u);

    // A valid binary image is consumed byte-for-byte with no conversion.
    auto custom=airxonix::makeLegacyDefaultHighScores();
    custom.blocks[4].names[2]={{'A','I','R','X','O','N','I','X',' ',' ',' ',' ',' ',' ',' ',' '}};
    custom.blocks[4].values[2]=0x12345678u;
    assert(airxonix::saveLegacyHighScores(path,custom));
    Game c;
    assert(c.initializeLegacyHighScores(path));
    assert(c.legacyHighScores().blocks[4].names[2]==custom.blocks[4].names[2]);
    assert(c.legacyHighScores().blocks[4].values[2]==0x12345678u);

    std::filesystem::remove_all(root,ec);
    std::cout << "highscore persistence r239 PASS\n";
}

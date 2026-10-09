#include "core/legacy_highscore.hpp"

#include <cassert>
#include <cstdint>
#include <filesystem>

using namespace airxonix;

int main() {
    static_assert(sizeof(LegacyHighScoreFile) == 0x640);
    static_assert(sizeof(LegacyHighScoreBlock) == 0xC8);
    assert(LegacyHighScoreTrace::loadRoutine == 0x0040F010u);
    assert(LegacyHighScoreTrace::saveRoutine == 0x0040EFF0u);
    assert(LegacyHighScoreTrace::fileReadHelper == 0x00409630u);
    assert(LegacyHighScoreTrace::fileWriteHelper == 0x004096A0u);
    assert(LegacyHighScoreTrace::valuesOffsetInBlock == 0xA0u);

    const auto defaults = makeLegacyDefaultHighScores();
    for (const auto& block : defaults.blocks) {
        for (const auto& name : block.names)
            for (char c : name) assert(c == '.');
        for (auto v : block.values) assert(v == 0u);
    }

    assert(LegacySoundInfoResourceTrace::loader == 0x004230F0u);
    assert(LegacySoundInfoResourceTrace::bytes == 0x1CA4u);
    assert(LegacySoundInfoResourceTrace::resourceName == "SOUNDINF");
    assert(LegacySoundInfoResourceTrace::resourceType == "BIN");
    assert(LegacyHighScoreTrace::reachableBlocks == 5u);
    assert(LegacyHighScoreTrace::reachableBlockNames[0] == "ПРОСТАЯ");
    assert(LegacyHighScoreTrace::reachableBlockNames[4] == "ЭКСТРИМ");
    assert(LegacyRegistrationTrace::readRoutine == 0x0040B9C0u);
    assert(LegacyRegistrationTrace::writeRoutine == 0x0040BAB0u);
    assert(LegacyRegistrationTrace::registryKey == "Software\\AxySoft\\AirXonix");
    assert(LegacyRegistrationTrace::nameValue == "RegName");
    assert(LegacyRegistrationTrace::codeValue == "RegCode");

    const auto tmp = std::filesystem::temp_directory_path() / "airxonix-r51-hscore.bin";
    assert(saveLegacyHighScores(tmp, defaults));
    assert(std::filesystem::file_size(tmp) == 0x640u);
    LegacyHighScoreFile loaded{};
    assert(loadLegacyHighScores(tmp, loaded));
    assert(loaded.blocks[0].names[0][0] == '.');
    std::filesystem::remove(tmp);
    return 0;
}

#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>


// DIRECT EXE r51: exact header of embedded SOUNDINF/BIN after 0x4230F0 copies
// it to 0x025B5BC8.  The first DWORD is modeCount=5.  Five CP1251 names of
// 12 bytes begin at +0x04.  Level counts at +0x64 and cumulative starts at
// +0x84 are indexed by the mode selectors and gameplay wrapper.  The 82
// 28-byte records at +0xA4 are the source of legacy_levels.inc.
struct LegacyModeResourceTrace {
    static constexpr std::uintptr_t loader = 0x004230F0u;
    static constexpr std::uintptr_t runtimeBase = 0x025B5BC8u;
    static constexpr std::size_t resourceBytes = 0x1CA4u;
    static constexpr std::size_t modeCountOffset = 0x00u;
    static constexpr std::size_t modeNamesOffset = 0x04u;
    static constexpr std::size_t modeNameBytes = 12u;
    static constexpr std::size_t modeCountsOffset = 0x64u;
    static constexpr std::size_t modeStartsOffset = 0x84u;
    static constexpr std::size_t levelRecordsOffset = 0xA4u;
    static constexpr std::size_t modeCount = 5u;
    static constexpr std::size_t totalLevels = 82u;
    static constexpr std::size_t levelRecordBytes = 28u;
    // r157 DIRECT EXE: resourceBytes-levelRecordsOffset == 0x1C00, exactly
    // 256 record-sized regions, but levelRecordsOffset runtime base 0x25B5C6C
    // has exactly one code xref (0x424EE9). That xref computes
    // (modeStart + levelIndex) * 28, and the five mode start/count pairs cap
    // the reachable index at 81. The remaining 174 physical regions are
    // unreachable payload/capacity in the final executable, not hidden levels.
    static constexpr std::size_t physicalRecordCapacity = 256u;
    static constexpr std::size_t normalLevelRecords = 82u;
    static constexpr std::size_t unreachableTailRecordRegions = 174u;
    static constexpr std::size_t opaqueTailRecordRegions = unreachableTailRecordRegions; // historical alias
    static constexpr std::uintptr_t soleLevelRecordBaseXref = 0x00424EE9u;
    static constexpr std::size_t maxReachableRecordIndex = 81u;
    static constexpr std::array<unsigned, modeCount> levelCounts{{7,15,20,20,20}};
    static constexpr std::array<unsigned, modeCount> levelStarts{{0,7,22,42,62}};
    static constexpr std::array<std::string_view, modeCount> modeNames{{
        "ПРОСТАЯ", "КЛАССИКА", "МОДЕРН", "ХАРД", "ЭКСТРИМ"
    }};
    static constexpr std::uintptr_t levelPointerBuild = 0x00424EE9u;
    static constexpr std::uintptr_t levelConsumer = 0x00418DD0u;
};

struct LevelRecord {
    uint8_t enemySpeed = 0;
    uint8_t enemyTypeACount = 0;
    uint8_t enemyTypeBCount = 0;
    uint8_t crawlerSpeed = 0;
    uint8_t crawlerCount = 0;
    uint8_t specialHoming = 0;
    uint8_t specialEraser = 0;
    uint8_t legacyCheckByte = 0; // observed but deliberately not used by native port
    std::array<uint8_t,5> shapeType{};
    std::array<uint8_t,5> shapeX{};
    std::array<uint8_t,5> shapeY{};
    std::array<uint8_t,5> shapeRadius{};
};

struct LevelMode {
    std::string name;
    std::vector<LevelRecord> levels;
};

class LevelDatabase {
public:
    LevelDatabase();
    explicit LevelDatabase(const std::vector<std::uint8_t>& soundInf);
    const std::vector<LevelMode>& modes() const { return modes_; }
    const LevelRecord& level(std::size_t mode, std::size_t index) const;
    std::size_t totalLevels() const;
private:
    std::vector<LevelMode> modes_;
};

std::vector<std::uint8_t> makeCleanroomSoundInf(); // r205 standalone deterministic fallback

float legacyDifficultyScale(float configValue); // config default 800 -> 1.0
float legacyScoreMultiplier(const LevelRecord& r, float difficultyScale);

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string_view>

namespace airxonix {

// DIRECT EXE r50: 0x40F010/0x40EFF0 load/save exactly 0x640 bytes from
// "hscore.bin" at runtime buffer 0x025450D0.  The default initializer at
// 0x40F02C..0x40F08C proves 8 blocks of 200 bytes.  Each block contains ten
// 16-byte name records followed by ten 32-bit values.
struct LegacyHighScoreBlock {
    std::array<std::array<char, 16>, 10> names{};
    std::array<std::uint32_t, 10> values{};
};
static_assert(sizeof(LegacyHighScoreBlock) == 200);

struct LegacyHighScoreFile {
    std::array<LegacyHighScoreBlock, 8> blocks{};
};
static_assert(sizeof(LegacyHighScoreFile) == 0x640);

struct LegacyHighScoreTrace {
    static constexpr std::string_view fileName = "hscore.bin";
    static constexpr std::uintptr_t runtimeBuffer = 0x025450D0u;
    static constexpr std::uintptr_t loadRoutine = 0x0040F010u;
    static constexpr std::uintptr_t saveRoutine = 0x0040EFF0u;
    static constexpr std::uintptr_t fileReadHelper = 0x00409630u;
    static constexpr std::uintptr_t fileWriteHelper = 0x004096A0u;
    static constexpr std::size_t fileSize = 0x640u;
    static constexpr std::size_t blockCount = 8u;
    static constexpr std::size_t blockSize = 0xC8u;
    static constexpr std::size_t entriesPerBlock = 10u;
    static constexpr std::size_t nameBytes = 16u;
    static constexpr std::size_t valuesOffsetInBlock = 0xA0u;
    static constexpr std::uint32_t defaultNameDword = 0x2E2E2E2Eu; // "...."
    // r51: only blocks 0..4 are reachable through the normal selector.  The
    // second argument of 0x40F160 is the selected mode returned by
    // 0x4112F0/0x411870; those selectors are bounded by SOUNDINF modeCount=5.
    static constexpr std::size_t reachableBlocks = 5u;
    static constexpr std::array<std::string_view, reachableBlocks> reachableBlockNames{{
        "ПРОСТАЯ", "КЛАССИКА", "МОДЕРН", "ХАРД", "ЭКСТРИМ"
    }};
    static constexpr std::uintptr_t copyBlockToWorking = 0x0040F0A0u;
    static constexpr std::uintptr_t highScoreScreen = 0x0040F160u;
    static constexpr std::uintptr_t mainMenuHighScoreCall = 0x00413163u;
    static constexpr std::uintptr_t postGameplayHighScoreCall = 0x00424F48u;
};

LegacyHighScoreFile makeLegacyDefaultHighScores();
bool loadLegacyHighScores(const std::filesystem::path& path, LegacyHighScoreFile& out);
bool saveLegacyHighScores(const std::filesystem::path& path, const LegacyHighScoreFile& value);

// r240 DIRECT EXE 0x40F19E..0x40F252. Returns the inserted row [0,9], or -1
// when score <= the current tenth place. Equal scores remain stable: a new
// score is inserted after all equal values that precede the insertion point.
int insertLegacyHighScoreCandidate(LegacyHighScoreBlock& block,std::uint32_t score);

// r241 DIRECT EXE 0x40F4E4..0x40F674. The original name editor works on the
// fixed 16-byte name record inserted by 0x40F160. It keeps a separate typed
// length; untouched/trailing cells remain literal '.'.
enum class LegacyHighScoreNameAction {
    None,
    Edited,
    Confirmed,
    Cancelled,
};

struct LegacyHighScoreNameEditResult {
    LegacyHighScoreNameAction action = LegacyHighScoreNameAction::None;
    int sfx = -1; // -1=no SFX, otherwise original SFX id (0x15/0x16)
};

// `code` is the byte/code returned by the legacy input routine 0x409480.
// `typedLength` is the separate [0,16] cursor/length used by the EXE.
LegacyHighScoreNameEditResult applyLegacyHighScoreNameInput(
    std::array<char,16>& name,int& typedLength,int code);

// PortMaster/Anbernic extension r247. The PC keyboard editor above remains
// untouched. Handhelds have no physical keyboard, so D-Pad Up/Down cycles a
// selected fixed-width cell through the filler dot plus Russian uppercase
// alphabet in Windows-1251 order (including Ё), while Left/Right selects the
// cell. This helper changes one byte only; persistence remains the original
// 16-byte CP1251 high-score format.
char cycleLegacyHighScoreDpadChar(char current,int direction);
int legacyHighScoreVisibleNameLength(const std::array<char,16>& name);

// DIRECT EXE r51 CORRECTION TO r50:
// SOUNDINF is not the SFX metadata table.  It is the central mode/level
// descriptor resource.  Its exact layout is documented by
// LegacyModeResourceTrace in game/level.hpp.  Keep these loader anchors here
// because they are also useful when reproducing the original resource load.
struct LegacySoundInfoResourceTrace {
    static constexpr std::uintptr_t loader = 0x004230F0u;
    static constexpr std::uintptr_t runtimeBase = 0x025B5BC8u;
    static constexpr std::string_view resourceName = "SOUNDINF";
    static constexpr std::string_view resourceType = "BIN";
    static constexpr std::size_t dwordCount = 0x729u;
    static constexpr std::size_t bytes = dwordCount * sizeof(std::uint32_t);
};

struct LegacyRegistrationTrace {
    static constexpr std::uintptr_t readRoutine = 0x0040B9C0u;
    static constexpr std::uintptr_t writeRoutine = 0x0040BAB0u;
    static constexpr std::string_view registryKey = "Software\\AxySoft\\AirXonix";
    static constexpr std::string_view nameValue = "RegName";
    static constexpr std::string_view codeValue = "RegCode";
    static constexpr std::uintptr_t nameBuffer = 0x0044DEE0u;
    static constexpr std::uintptr_t codeBuffer = 0x0044DF20u;
    static constexpr std::uintptr_t nameLength = 0x0044DF50u;
    static constexpr std::uintptr_t codeLength = 0x0044DF54u;
};

} // namespace airxonix

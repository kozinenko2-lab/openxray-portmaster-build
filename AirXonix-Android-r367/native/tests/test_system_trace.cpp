#include "audio/legacy_audio_trace.hpp"
#include "core/legacy_highscore.hpp"
#include "game/level.hpp"

#include <cassert>
#include <string_view>

using namespace airxonix;

int main() {
    static_assert(LegacyModeResourceTrace::modeCount == 5);
    static_assert(LegacyModeResourceTrace::totalLevels == 82);
    static_assert(LegacyModeResourceTrace::levelRecordBytes == 28);
    assert(LegacyModeResourceTrace::runtimeBase == 0x025B5BC8u);
    assert(LegacyModeResourceTrace::modeCountsOffset == 0x64u);
    assert(LegacyModeResourceTrace::modeStartsOffset == 0x84u);
    assert(LegacyModeResourceTrace::levelRecordsOffset == 0xA4u);
    assert((LegacyModeResourceTrace::levelCounts == std::array<unsigned,5>{{7,15,20,20,20}}));
    assert((LegacyModeResourceTrace::levelStarts == std::array<unsigned,5>{{0,7,22,42,62}}));
    assert(LegacyModeResourceTrace::modeNames[0] == "ПРОСТАЯ");
    assert(LegacyModeResourceTrace::modeNames[4] == "ЭКСТРИМ");

    assert(LegacyHighScoreTrace::reachableBlocks == 5u);
    assert(LegacyHighScoreTrace::reachableBlockNames == LegacyModeResourceTrace::modeNames);
    assert(LegacyHighScoreTrace::copyBlockToWorking == 0x0040F0A0u);
    assert(LegacyHighScoreTrace::mainMenuHighScoreCall == 0x00413163u);
    assert(LegacyHighScoreTrace::postGameplayHighScoreCall == 0x00424F48u);

    assert(LegacySfxTrace::logicalSlots == 64u);
    assert(LegacySfxTrace::fourcc[0x0A] == "bon0");
    assert(LegacySfxTrace::fourcc[0x0B] == "bon1");
    assert(LegacySfxTrace::fourcc[0x0C] == "bon2");
    assert(LegacySfxTrace::fourcc[0x0D] == "bon3");
    assert(LegacySfxTrace::fourcc[0x15] == "clc1");
    assert(LegacySfxTrace::fourcc[0x16] == "clc2");
    assert(LegacySfxTrace::fourcc[0x17] == "comp");
    assert(LegacySfxTrace::fourcc[0x19] == "game");
    assert(LegacySfxTrace::fourcc[0x20] == "life");
    assert(LegacySfxTrace::fourcc[0x21] == "bonu");
    assert(LegacySfxTrace::fourcc[0x23] == "bour");
    assert(LegacySfxTrace::fourcc[0x24] == "time");
    assert(LegacySfxTrace::fourcc[0x25] == "slow");
    assert(LegacySfxTrace::fourcc[0x26] == "acce");
    assert(LegacySfxTrace::fourcc[0x31] == "whip");
    assert(LegacySfxTrace::fourcc[53].empty());

    assert(LegacyWavePackTrace::bankPath == "music\\29.mus");
    assert(LegacyMusicTrace::trackCount == 10u);
    assert(LegacyMusicTrace::firstTrack == 0 && LegacyMusicTrace::lastTrack == 9);
    assert(LegacyMusicTrace::gameplaySelector == 0x00422EE0u);
    assert(LegacyMusicTrace::pathTemplate == "music\\00.mus");
    return 0;
}

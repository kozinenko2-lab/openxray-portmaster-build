#pragma once

#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace airxonix {

// DIRECT EXE r51.  These are the 64 logical SFX ids consumed by 0x40AE50 /
// 0x40AE90.  0x423140 fills the FourCC lookup table at 0x4513F0 and then
// tail-jumps to 0x40AE00, which resolves each non-zero FourCC through the
// WAVEPACK table produced by 0x409BE0/0x409CC0.  Empty entries are deliberate.
struct LegacySfxTrace {
    static constexpr std::uintptr_t fourccInit = 0x00423140u;
    static constexpr std::uintptr_t resolveTable = 0x0040AE00u;
    static constexpr std::uintptr_t playSimple = 0x0040AE50u;
    static constexpr std::uintptr_t playSpatial = 0x0040AE90u;
    static constexpr std::uintptr_t fourccTable = 0x004513F0u;
    static constexpr std::uintptr_t resolvedTable = 0x0045164Cu;
    static constexpr std::size_t logicalSlots = 64u;

    static constexpr std::array<std::string_view, logicalSlots> fourcc = {{
        "min0","min0","bol1","bol2","fire","fir1","up01","vint",
        "sfly","efly","bon0","bon1","bon2","bon3","fir1","bfly",
        "bpop","fir2","tick","bvzr","levl","clc1","clc2","comp",
        "haha","game","welk","cow2","cow4","cow1","cow3","gove",
        "life","bonu","beep","bour","time","slow","acce","out!",
        "byeb","strt","lets","aaaa","ohoh","yes1","cool","that",
        "sur1","whip","whip","cmex","oyoy","","","",
        "","","","","","","",""
    }};
};

// DIRECT EXE r51.  music\\29.mus is mapped by 0x409BE0 as the raw mono SFX
// sample bank.  Its record directory is the embedded WAVEPACK/BIN resource;
// 0x409C51 walks up to 64 8-byte {FourCC,length} entries and builds a 12-byte
// lookup record.  `levl` and `game` have playback length shortened by 2000
// samples at 0x409C75/0x409C89 while the bank cursor advances by raw length.
struct LegacyWavePackTrace {
    static constexpr std::uintptr_t bankOpenRoutine = 0x00409BE0u;
    static constexpr std::uintptr_t lookupRoutine = 0x00409CC0u;
    static constexpr std::string_view bankPath = "music\\29.mus";
    static constexpr std::size_t maximumDirectoryEntries = 64u;
    static constexpr std::size_t directoryRecordBytes = 8u;
    static constexpr std::size_t runtimeRecordBytes = 12u;
    static constexpr std::uint32_t shortenedSamples = 2000u;
};

// DIRECT EXE r51.  0x40AFC0 edits the two digits of the literal
// "music\\00.mus" and maps the selected track.  0x422EE0 is the gameplay
// selector: it operates on ten usage counters, chooses among the least-used
// counters and returns a track id 0..9.  Thus normal music files are exactly
// 00.mus .. 09.mus; 29.mus belongs to the SFX bank above.
struct LegacyMusicTrace {
    static constexpr std::uintptr_t openTrackRoutine = 0x0040AFC0u;
    static constexpr std::uintptr_t requestTrackRoutine = 0x0040B240u;
    static constexpr std::uintptr_t updateRoutine = 0x0040B0D0u;
    static constexpr std::uintptr_t gameplaySelector = 0x00422EE0u;
    static constexpr std::uintptr_t selectorReset = 0x00422EA0u;
    static constexpr std::uintptr_t usageCounters = 0x025B78A4u;
    static constexpr std::size_t trackCount = 10u;
    static constexpr int firstTrack = 0;
    static constexpr int lastTrack = 9;
    static constexpr std::string_view pathTemplate = "music\\00.mus";
};

// r165 DIRECT EXE: literal 0x00422EE0 selector. After finding the minimum
// of ten use counters, the original MSVC code does `rand() & 0x15`, then +1.
// It scans cyclically in the order 1,2,...,9,0 until that many minimum-use
// records have been encountered, increments the chosen counter, and returns it.
// Note that 0x15 is a bit mask, deliberately NOT modulo 10.
struct LegacyMusicSelectorTrace {
    static constexpr std::uintptr_t routine=0x00422EE0u;
    static constexpr std::uintptr_t resetRoutine=0x00422EA0u;
    static void reset(std::array<std::uint32_t,10>& usage){
        usage.fill(0u); usage[0]=1u;
    }
    static std::size_t select(std::array<std::uint32_t,10>& usage,int randomValue){
        const std::uint32_t minUse=*std::min_element(usage.begin(),usage.end());
        const int wanted=(static_cast<unsigned>(randomValue)&0x15u)+1;
        int seen=0; std::size_t idx=0;
        for(;;){
            idx=(idx+1u)%10u;
            if(usage[idx]==minUse && ++seen>=wanted){ ++usage[idx]; return idx; }
        }
    }
};


// DIRECT EXE r54. 0x409F51..0x409F9B constructs the exact WAVEFORMATEX
// used by the legacy DirectSound output path. The fields decode to PCM,
// stereo, 22050 Hz, 8-bit, 2-byte block alignment and 44100 bytes/sec.
// This is the native format expected by the original software mixer; no
// 44.1-kHz/16-bit assumption is justified for compatibility playback.
struct LegacyDirectSoundFormatTrace {
    static constexpr std::uintptr_t formatBuildBegin = 0x00409F51u;
    static constexpr std::uintptr_t formatBuildEnd = 0x00409F9Bu;
    static constexpr std::uint16_t formatTag = 1u;
    static constexpr std::uint16_t channels = 2u;
    static constexpr std::uint32_t samplesPerSecond = 22050u;
    static constexpr std::uint32_t averageBytesPerSecond = 44100u;
    static constexpr std::uint16_t blockAlign = 2u;
    static constexpr std::uint16_t bitsPerSample = 8u;
    static constexpr std::uint16_t cbSize = 0u;
};

// DIRECT EXE r52.  The legacy mixer has seven concurrent 32-byte SFX voices.
// 0x40AEF0 allocates the first free slot, 0x40AF70 updates a live voice's
// position/attenuation parameters and 0x40AFA0 clears one.  0x40A890 mixes all
// seven active voices each audio tick.  Simple SFX use equal L/R gains derived
// from master SFX volume * 63.  Spatial SFX run through 0x40A7B0; the two
// channel gains are clamped independently to 0x55 (85) before mixing.
// DIRECT EXE r57. 0x409DF0 builds a 96x256 signed contribution lookup
// table at 0x00457A54. The outer row index is multiplied by the exact float
// 0.015625 (1/64), the unsigned source byte is centered by 128, and helper
// 0x43129C converts with x87 truncation-toward-zero. 0x40A6D0/0x40A640 then
// index rows by left/right gain and add the resulting signed contributions.
struct LegacyGainTableTrace {
    static constexpr std::uintptr_t buildRoutine = 0x00409DF0u;
    static constexpr std::uintptr_t conversionHelper = 0x0043129Cu;
    static constexpr std::uintptr_t tableBase = 0x00457A54u;
    static constexpr std::uintptr_t tableEnd = 0x0046FA54u;
    static constexpr std::uintptr_t mixChunkRoutine = 0x0040A6D0u;
    static constexpr std::size_t gainRows = 96u;
    static constexpr std::size_t sampleValues = 256u;
    static constexpr float rowScale = 0.015625f;
    static constexpr int center = 128;
    static constexpr int divisor = 64;
    static constexpr int contribution(int gain, int sample) {
        const int product=(sample-center)*gain;
        return product/divisor; // C++ integer division truncates toward zero.
    }
};

struct LegacySfxMixerTrace {
    static constexpr std::uintptr_t mixerRoutine = 0x0040A890u;
    static constexpr std::uintptr_t spatialGainRoutine = 0x0040A7B0u;
    static constexpr std::uintptr_t voiceAllocator = 0x0040AEF0u;
    static constexpr std::uintptr_t voiceUpdate = 0x0040AF70u;
    static constexpr std::uintptr_t voiceStop = 0x0040AFA0u;
    static constexpr std::uintptr_t voicePool = 0x00451548u;
    static constexpr std::size_t voiceCount = 7u;
    static constexpr std::size_t voiceStride = 0x20u;
    static constexpr int simpleGainScale = 63;
    static constexpr int spatialGainClamp = 0x55;
    static constexpr std::uintptr_t listenerX = 0x00451534u;
    static constexpr std::uintptr_t listenerY = 0x00451538u;
    static constexpr std::uintptr_t listenerZ = 0x0045153Cu;

    // DIRECT EXE r53: exact 32-byte slot layout consumed by 0x40A890.
    static constexpr std::size_t sampleBaseOffset = 0x00u;
    static constexpr std::size_t sampleEndOffset = 0x04u;
    static constexpr std::size_t cursorOffset = 0x08u;
    static constexpr std::size_t reservedOffset = 0x0Cu;
    static constexpr bool reservedWrittenByAllocator = false;
    static constexpr bool reservedWrittenByUpdater = false;
    static constexpr bool reservedReadByMixer = false;
    static constexpr std::size_t worldXOffset = 0x10u;
    static constexpr std::size_t worldYOffset = 0x14u;
    static constexpr std::size_t worldZOffset = 0x18u;
    static constexpr std::size_t attenuationOffset = 0x1Cu;

    // The allocator is strictly first-free. It has no victim-selection/steal
    // path. If all seven slots are occupied the legacy x86 routine advances
    // one slot beyond the pool and returns handle 8, an original overflow
    // edge case. Native code must reject exhaustion rather than reproduce the
    // out-of-bounds write.
    static constexpr bool hasVoiceStealing = false;
    static constexpr std::size_t legacyOverflowIndex = 7u;
    static constexpr int legacyOverflowHandle = 8;
};

// DIRECT EXE r52.  Music playback is mixed into the same software output path,
// but has independent fade state.  0x40B240 stores {pendingTrack,fadeScalar};
// 0x40B260 starts a fade-out by writing -arg to the fade-rate.  0x40B0D0
// integrates currentGain += dt*fadeRate, clamps to [0,1], closes the old mapped
// track at zero, and opens the pending track once the stream side is ready.
// 0x40AFC0 maps the selected music\\NN.mus file and takes PCM base/length from
// mapped-file object offsets +0x10/+0x14.  It probes the payload in 0x400-byte
// steps before exposing it to the mixer.
struct LegacyMusicStreamTrace {
    static constexpr std::uintptr_t openTrack = 0x0040AFC0u;
    static constexpr std::uintptr_t closeTrack = 0x0040B0A0u;
    static constexpr std::uintptr_t updateFade = 0x0040B0D0u;
    static constexpr std::uintptr_t requestTrack = 0x0040B240u;
    static constexpr std::uintptr_t beginFadeOut = 0x0040B260u;
    static constexpr std::uintptr_t mappedPcmBase = 0x004513D4u;
    static constexpr std::uintptr_t mappedCursor = 0x004513E0u;
    static constexpr std::uintptr_t mappedEnd = 0x004513E8u;
    static constexpr std::uintptr_t pendingTrack = 0x0043FCE4u;
    static constexpr std::uintptr_t currentGain = 0x0051BECCu;
    static constexpr std::uintptr_t fadeRate = 0x0051BED0u;
    static constexpr std::size_t mappedObjectDataOffset = 0x10u;
    static constexpr std::size_t mappedObjectLengthOffset = 0x14u;
    static constexpr std::size_t probeStrideBytes = 0x400u;

    // DIRECT EXE r53: 0x4097C0 maps the whole file and stores MapViewOfFile
    // base at object+0x10 and GetFileSize result at object+0x14. 0x40AFC0
    // publishes those values directly; no MUS header/container parser exists.
    // The mixer wraps cursor->base when a requested chunk reaches/passes end,
    // so normal music tracks are raw mapped PCM/data streams with seamless
    // looping at the byte boundary.
    static constexpr std::uintptr_t mapOpenRoutine = 0x004097C0u;
    static constexpr std::uintptr_t mixerRoutine = 0x0040A890u;
    static constexpr std::uintptr_t streamActiveFlag = 0x0051BEC0u;
    static constexpr std::uintptr_t streamIdleHandshake = 0x0043FC7Cu;
    static constexpr bool parsesContainerHeader = false;
    static constexpr bool loopsAtMappedEnd = true;
};



// DIRECT EXE r146. 0x40B280 stores a two-channel music modulation pair.
// Gameplay (0x41959C..0x4195C9) and finale (0x41B7A9..0x41B7D8) derive it
// from Xonix world X; menu/session setup writes (1,1).
struct LegacyMusicStereoTrace {
    static constexpr std::uintptr_t setter = 0x0040B280u;
    static constexpr std::uintptr_t gameplayCaller = 0x004195C4u;
    static constexpr std::uintptr_t finaleCaller = 0x0041B7D3u;
    static constexpr std::uintptr_t menuResetCaller = 0x00412FD1u;
    static constexpr std::uintptr_t sessionResetCaller = 0x00424F65u;
    static constexpr float centerX = 0.5f;
    static constexpr float centerGain = 0.800000011920929f;
    static constexpr float xScale = 3.0f;
    static constexpr float channel0(float x){return centerGain+(x-centerX)*xScale;}
    static constexpr float channel1(float x){return centerGain-(x-centerX)*xScale;}
};

// DIRECT EXE r96.  Final-second death audio owned by 0x41C030.
// With lives remaining, the first frame below 1000 ms starts logical SFX 7
// (FourCC "vint") as a spatial voice at the Xonix position, y=.1, scalar=1,
// stores the returned handle at 0x0257F4E8 and follows the descending Y through
// 0x40AF70.  With zero lives, the branch instead plays simple SFX 0x18
// (FourCC "haha") once and starts music fade-out at 0.0003000000142492354/ms.
struct LegacyDeathAudioTrace {
    static constexpr std::uintptr_t deathRoutine = 0x0041C030u;
    static constexpr std::uintptr_t entrySpatialOneShot = 0x0041C04Bu;
    static constexpr std::uintptr_t entryRetainedVoice = 0x0041C08Eu;
    static constexpr std::uintptr_t retainedVoiceStop = 0x0041C3DFu;
    static constexpr std::uintptr_t impactSpatialOneShot = 0x0041C3F1u;
    static constexpr std::uintptr_t splitAtFinalSecond = 0x0041C540u;
    static constexpr std::uintptr_t respawnVoiceStart = 0x0041C569u;
    static constexpr std::uintptr_t respawnVoiceUpdate = 0x0041C60Fu;
    static constexpr std::uintptr_t zeroLivesOneShot = 0x0041C638u;
    static constexpr std::uintptr_t zeroLivesMusicFade = 0x0041C644u;
    static constexpr std::uintptr_t respawnVoiceHandle = 0x0257F4E8u;
    static constexpr std::size_t entryLogicalSfxId = 0x05u;
    static constexpr std::string_view entryFourcc = "fir1";
    static constexpr std::size_t retainedLogicalSfxId = 0x11u;
    static constexpr std::string_view retainedFourcc = "fir2";
    static constexpr std::size_t impactLogicalSfxId = 0x04u;
    static constexpr std::string_view impactFourcc = "fire";
    static constexpr float entryAttenuation = 1.0f;
    static constexpr float impactAttenuation = 1.2999999523162842f;
    static constexpr int finalSecondThresholdMs = 1000;
    static constexpr std::size_t respawnLogicalSfxId = 0x07u;
    static constexpr std::string_view respawnFourcc = "vint";
    static constexpr float respawnInitialY = 0.10000000149011612f;
    static constexpr float respawnAttenuation = 1.0f;
    static constexpr std::size_t zeroLivesLogicalSfxId = 0x18u;
    static constexpr std::string_view zeroLivesFourcc = "haha";
    static constexpr float zeroLivesMusicFadePerMs = 0.0003000000142492354f;
};


// DIRECT EXE r98. 0x025B7898 is the spoken/announcer enable global used by
// the death sequence. It gates only voice samples in this preserved CFG:
// out!, aaaa, ohoh, oyoy and gove. The early cue fires once after remaining
// time becomes strictly < 3850 ms, advances 0x0257F548 as (index+1)&3 only
// when speech is enabled, and submits a positional sound with scalar 1.5.
// The Game Over cue uses the same enable global but simple 0x40AE50 playback.
struct LegacyDeathSpeechTrace {
    static constexpr std::uintptr_t speechEnableGlobal = 0x025B7898u;
    static constexpr std::uintptr_t speechCycleGlobal = 0x0257F548u;
    static constexpr std::uintptr_t earlyThresholdCompare = 0x0041C1BBu;
    static constexpr int earlyThresholdRemainingMs = 3850;
    static constexpr std::uintptr_t earlyGate = 0x0041C1C6u;
    static constexpr std::uintptr_t earlyCycleAdvance = 0x0041C1D2u;
    static constexpr std::uintptr_t gameOverGate = 0x0041C6C9u;
    static constexpr std::uintptr_t gameOverPlay = 0x0041C6DAu;
    static constexpr float earlySpatialScalar = 1.5f;
    static constexpr std::array<std::size_t,4> earlyLogicalIds{{0x27u,0x2Bu,0x2Cu,0x34u}};
    static constexpr std::array<std::string_view,4> earlyFourcc{{"out!","aaaa","ohoh","oyoy"}};
    static constexpr std::size_t gameOverLogicalId = 0x1Fu;
    static constexpr std::string_view gameOverFourcc = "gove";
};


// DIRECT EXE r99. Exit audio at the two terminal paths of 0x41C030.
// Normal lives>0 completion reaches 0x41CE2C: positional ID 9 / efly at the
// final Xonix XYZ, scalar 1.0, then 0x40AFA0 stops the retained vint handle.
// Zero-lives event exit reaches 0x41CE87 and plays simple ID 0x16 / clc2;
// the -60000 ms timeout exits at 0x41CE7C and does not play clc2.
struct LegacyDeathExitAudioTrace {
    static constexpr std::uintptr_t respawnExit = 0x0041CE2Cu;
    static constexpr std::uintptr_t respawnSpatialPlay = 0x0041CE43u;
    static constexpr std::uintptr_t respawnVoiceStop = 0x0041CE58u;
    static constexpr std::size_t respawnExitLogicalId = 0x09u;
    static constexpr std::string_view respawnExitFourcc = "efly";
    static constexpr float respawnExitScalar = 1.0f;
    static constexpr std::uintptr_t gameOverEventExit = 0x0041CE87u;
    static constexpr std::size_t gameOverEventLogicalId = 0x16u;
    static constexpr std::string_view gameOverEventFourcc = "clc2";
    static constexpr std::uintptr_t gameOverTimeoutExit = 0x0041CE7Cu;
    static constexpr bool timeoutPlaysExitSfx = false;
};

} // namespace airxonix

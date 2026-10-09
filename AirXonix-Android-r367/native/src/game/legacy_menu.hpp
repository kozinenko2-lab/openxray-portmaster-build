#pragma once
#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>

enum class LegacyMenuDispatchKind {
    Call,
    CallWithConfiguredArgument,
    ReturnFromMenu
};

struct LegacyMenuEntryTrace {
    int index;
    LegacyMenuDispatchKind kind;
    std::uint32_t targetAddress;
};

// Direct control-flow transcription from AirXonix.wrp.exe SHA-256
// 73e9b69646862548a3bf3cfc3d710261a8a0f7884fe155d7c649b7e3a2e88030.
// 0x00412F90 keeps the selected entry in EBP and clamps it to [0,4].
inline constexpr std::array<LegacyMenuEntryTrace,5> kLegacyMainMenuEntries{{
    {0,LegacyMenuDispatchKind::Call,0x00424DE0u},
    {1,LegacyMenuDispatchKind::Call,0x00413670u},
    {2,LegacyMenuDispatchKind::CallWithConfiguredArgument,0x0040F160u},
    {3,LegacyMenuDispatchKind::Call,0x00410CF0u},
    {4,LegacyMenuDispatchKind::ReturnFromMenu,0x00413662u}
}};

// Direct constructor xrefs recovered from the same binary.
inline constexpr std::uint32_t kLegacyGameplayEntryAddress=0x00424DE0u;
inline constexpr std::uint32_t kLegacyGameplayHudConstructor=0x00423350u;
inline constexpr std::uint32_t kLegacyPostGameplayM1Constructor=0x00423C30u;
inline constexpr std::uint32_t kLegacyEntry1M2Constructor=0x00423EE0u;


// r37 direct caller-chain trace around the sole gameplay caller.
// 0x004195D0 (gameplay/session routine) returns to 0x00424EF1; its result is
// stored in ESI and the outer dispatcher eventually reaches 0x00424F3B where
// 0x00423C30 rebuilds the M1-family atlas. This proves the post-gameplay menu
// return is caller-owned rather than a direct call from 0x0041B2A0.
struct LegacyPostGameplayReturnTrace {
    std::uint32_t gameplayRoutine=0x004195D0u;
    std::uint32_t callerReturnSite=0x00424EF1u;
    std::uint32_t outerDispatchStart=0x00424EF9u;
    std::uint32_t postDispatchConstructorCall=0x00424F3Bu;
    std::uint32_t postGameplayConstructor=0x00423C30u;
    std::uint32_t dispatchEnd=0x00424F50u;
};
inline constexpr LegacyPostGameplayReturnTrace kLegacyPostGameplayReturn{};




struct LegacyMainMenuRuntime {
    static float approach(float v,float target,float maxDelta){
        if(v<target)return v+std::min(maxDelta,target-v);
        if(v>target)return v-std::min(maxDelta,v-target);
        return v;
    }
    static float approachBrightness(float v,bool selected,int dtMs){
        return approach(v,selected?1.0f:0.6f,float(dtMs)*0.002f);
    }
    static float approachScale(float v,bool selected,int dtMs){
        return approach(v,selected?1.15f:1.0f,float(dtMs)*0.001f);
    }
};


// r184 direct 0x4131FA..0x413296 / 0x41340F..0x413444.
// The selected index and the visible selector position are deliberately separate.
struct LegacyMainMenuSelectorSlideTrace {
    std::uint32_t targetBuildStart=0x004131FAu;
    std::uint32_t positiveApproachStart=0x0041321Fu;
    std::uint32_t negativeApproachStart=0x00413263u;
    std::uint32_t markerSubmitA=0x0041342Eu;
    std::uint32_t markerSubmitB=0x00413444u;
    float rowStep=-0.003000000026077032f;
    float approachPerMs=0.00004999999873689376f;
    float markerZBias=-0.001500000013038516f;
    static float targetFor(int selected){ return float(selected)*-0.003000000026077032f; }
    static float advance(float current,int selected,int dtMs){
        return LegacyMainMenuRuntime::approach(current,targetFor(selected),
            float(std::max(0,dtMs))*0.00004999999873689376f);
    }
};
inline constexpr LegacyMainMenuSelectorSlideTrace kLegacyMainMenuSelectorSlideTrace{};

// DIRECT EXE r62: exact least-used menu-environment selector at 0x00422F40.
// It finds the minimum usage counter among 8 records, takes (rand()&7)+1, and
// walks records in the peculiar legacy order 1,2,...,7,0, wrapping until the
// requested occurrence of a minimum-use record is reached. The selected
// record's counter is then incremented.
struct LegacyMenuThemeSelectorTrace {
    std::uint32_t routine=0x00422F40u;
    std::uint32_t table=0x00441830u;
    std::uint32_t firstUsage=0x0044183Cu;
    std::uint32_t randomCall=0x00422F5Du;
    std::uint32_t selectedCounterIncrement=0x00422F95u;
    int themeCount=8;
    int recordStride=16;

    static std::size_t select(std::array<std::uint32_t,8>& usage,int randomValue) {
        const auto minIt=std::min_element(usage.begin(),usage.end());
        const std::uint32_t minUse=*minIt;
        const int wanted=(randomValue&7)+1;
        int seen=0;
        std::size_t idx=0;
        for(;;){
            idx=(idx+1u)&7u; // exact scan order 1..7,0
            if(usage[idx]==minUse && ++seen>=wanted){
                ++usage[idx];
                return idx;
            }
        }
    }
};
inline constexpr LegacyMenuThemeSelectorTrace kLegacyMenuThemeSelectorTrace{};

// r185 DIRECT EXE: top-level gameplay wrapper 0x424DE0.
// This is the complete campaign/session dispatcher reached from main-menu entry 0.
struct LegacyGameplayCampaignTrace {
    static constexpr std::uint32_t routine=0x00424DE0u;
    static constexpr std::uint32_t musicFadeOut=0x00424DE9u;      // 0x40B260(.001)
    static constexpr std::uint32_t registrationFlag=0x025459B0u;
    static constexpr std::uint32_t unregisteredModeSelector=0x004112F0u;
    static constexpr std::uint32_t registeredModeSelector=0x00411870u;
    static constexpr std::uint32_t modeCancelValue=0xffffffffu;
    static constexpr std::uint32_t restartCredits=0x025B5B60u;    // initialized to 5
    static constexpr std::uint32_t musicUsageReset=0x00422EA0u;
    static constexpr std::uint32_t gameSpeedConfig=0x025B7878u;
    static constexpr std::uint32_t gameTimeScaleSink=0x00418D10u;
    static constexpr std::uint32_t levelCountBase=0x025B5C2Cu;
    static constexpr std::uint32_t levelStartBase=0x025B5C4Cu;
    static constexpr std::uint32_t levelRecordBase=0x025B5C6Cu;
    static constexpr std::uint32_t perLevelInit=0x00418DD0u;
    static constexpr std::uint32_t gameplayLoop=0x004195D0u;
    static constexpr std::uint32_t postGameplayMenuConstructor=0x00423C30u;
    static constexpr std::uint32_t highScoreScreen=0x0040F160u;
    static constexpr std::uint32_t returnMenuTrackRequest=0x0040B240u;
    static constexpr std::uint32_t returnStereoReset=0x0040B280u;

    // 0x4195D0 return contract as consumed by 0x424EF9..0x424F48.
    static constexpr int nextLevel=1;
    static constexpr int abortSession=-1;
    static constexpr int initialRestartCredits=5;

    static float gameTimeScale(float configValue){
        // 0x424E2D: (config - 800) * .00025 + 1.0.
        // 0x4194A0 multiplies timeGetTime() by this value before deriving dt,
        // so this is a gameplay clock/speed scale rather than a pure difficulty scalar.
        return (configValue-800.0f)*0.0002500000118743628f+1.0f;
    }
    static float difficultyScale(float configValue){ return gameTimeScale(configValue); } // historical alias
};
inline constexpr LegacyGameplayCampaignTrace kLegacyGameplayCampaignTrace{};

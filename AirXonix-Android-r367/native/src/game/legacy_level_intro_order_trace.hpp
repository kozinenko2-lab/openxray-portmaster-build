#pragma once
#include <cstdint>
namespace LegacyLevelIntroOrder {
struct Trace {
    static constexpr std::uintptr_t outerLevelInitCall=0x00424EF1u;
    static constexpr std::uintptr_t levelInitRoutine=0x00418DD0u;
    static constexpr std::uintptr_t gameplayRoutine=0x004195D0u;
    static constexpr std::uintptr_t introDispatchCall=0x00419602u;
    static constexpr std::uintptr_t introDispatcher=0x0041CEA0u;
    static constexpr std::uintptr_t firstGameplayTick=0x004196DFu;
    static constexpr std::uintptr_t restartKeySnapshot=0x004192B5u;
    static constexpr std::uintptr_t outerRestartLoop=0x00424E2Du;
    static constexpr bool introAfterLevelInit=true;
    static constexpr bool introBeforeTimerAndInput=true;
    static constexpr bool introRepeatsOnRestart=true;
    static constexpr bool introRepeatsOnNextLevel=true;
};
inline constexpr Trace kTrace{};
}

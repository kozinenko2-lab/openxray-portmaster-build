#pragma once
#include <cstdint>
// r334 DIRECT EXE: 0x40FF70 is the legacy AxySoft registration/anti-tamper
// violation screen, not game content. Caller 0x413135 reaches it only when
// registration/integrity global 0x025459B4 is non-zero after gameplay.
// The clean-room PortMaster intentionally does not reproduce this DRM UI.
struct LegacyRegistrationViolationTrace {
    std::uint32_t routine=0x0040FF70u;
    std::uint32_t callerGate=0x00413135u;
    std::uint32_t integrityGlobal=0x025459B4u;
    std::uint32_t heading=0x00441010u; // "LICENSE AGREEMENT VIOLATION !"
    std::uint32_t footer=0x00440DF0u;  // "PRESS ANY KEY"
    bool excludedFromCleanroomRuntime=true;
};
inline constexpr LegacyRegistrationViolationTrace kLegacyRegistrationViolationTrace{};

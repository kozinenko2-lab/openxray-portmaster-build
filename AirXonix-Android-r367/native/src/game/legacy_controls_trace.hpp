#pragma once
#include <array>
#include <cstdint>
#include <cmath>
#include <algorithm>

// DIRECT EXE r137: 0x00410D60 control-remap routine and 0x00440A40
// special-key table. Core stores original Win32/DirectInput-style legacy codes;
// platform code translates SDL events at the boundary.
struct LegacyControlsTrace {
    std::uint32_t routine=0x00410D60u;
    std::uint32_t bindingRight=0x025B7888u;
    std::uint32_t bindingLeft=0x025B788Cu;
    std::uint32_t bindingBackward=0x025B7890u;
    std::uint32_t bindingForward=0x025B7894u;
    std::uint32_t temporaryBindings=0x02545924u;
    std::uint32_t specialTableBegin=0x00440A40u;
    std::uint32_t specialTableEnd=0x00440D10u;
    std::array<int,4> defaults{{0x41,0x5A,0x58,0x43}}; // A,Z,X,C
    int forbiddenPauseKey=0x50; // P remains reserved for pause.
    int fadeMax=0x7C0;
    int fadeInRate=3;
    int fadeOutRate=-4;

    static constexpr float pulseRate=0.009999999776482582f;
    static constexpr float pulseAmplitude=90.0f;
    static constexpr float rowPulseBase=75.0f;
    static constexpr float promptPulseBase=70.0f;
    static constexpr float tau=6.2831853071795864769f;

    static float advancePulse(float phase,int dtMs){
        phase += float(std::max(0,dtMs))*pulseRate;
        while(phase>=tau)phase-=tau;
        return phase;
    }
    static int currentRowPulse(float phase){
        return int((std::cos(phase)+1.0f)*pulseAmplitude+rowPulseBase);
    }
    static int promptPulse(float phase){
        return int((std::sin(phase)+1.0f)*pulseAmplitude+promptPulseBase);
    }
    static std::uint32_t grayRgb(int v){
        const auto q=std::uint32_t(std::clamp(v,0,255)); return (q<<16)|(q<<8)|q;
    }
    static std::uint32_t promptRgb(int v){
        const auto q=std::uint32_t(std::clamp(v,0,255)); return 0x820082u|(q<<8);
    }


    // r332 DIRECT EXE 0x41124C..0x411254: the fnt4 grid uses 0x40BF40 with
    // palette level=(fadeCounter>>6). This is separate from 0x40C190(counter>>3).
    static constexpr int textPaletteLevel(int counter){
        const int c=counter<0?0:(counter>0x7C0?0x7C0:counter); return c>>6;
    }
    static constexpr int fadeTextByte(int byte,int counter){
        const int b=byte<0?0:(byte>255?255:byte); return (b*textPaletteLevel(counter))/31;
    }

    constexpr float modelLightScale(int counter) const {
        const int c=counter<0?0:(counter>fadeMax?fadeMax:counter);
        return float(c>>3)/255.0f;
    }

    constexpr bool alphaNumeric(int code) const {
        return (code>=0x30 && code<=0x39) || (code>=0x41 && code<=0x5A);
    }
    constexpr bool joystick1(int code) const { return code>=0x100 && code<=0x10D; }
    constexpr bool joystick2(int code) const { return code>=0x10E && code<=0x11B; }
    constexpr bool knownSpecial(int code) const {
        switch(code){
            case 0x70:case 0x71:case 0x72:case 0x73:case 0x74:case 0x75:case 0x76:case 0x77:case 0x78: // F1..F9
            case 0x09:case 0x10:case 0x21:case 0x22:case 0x23:case 0x24: // Tab Shift PgUp PgDn End Home
            case 0x60:case 0x61:case 0x62:case 0x63:case 0x64:case 0x65:case 0x66:case 0x67:case 0x68:case 0x69:
            case 0x6A:case 0x6B:case 0x6D:case 0x6E:case 0x6F:case 0x2D:case 0x2E:
                return true;
            default:return joystick1(code)||joystick2(code);
        }
    }
    constexpr bool assignable(int code) const {
        if(alphaNumeric(code)) return code!=forbiddenPauseKey;
        return knownSpecial(code);
    }
};
inline constexpr LegacyControlsTrace kLegacyControlsTrace{};

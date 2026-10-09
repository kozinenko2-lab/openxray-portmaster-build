#pragma once
#include <cstdint>
#include <cstddef>
struct LegacySettingsTrace {
    static constexpr const char* fileName="gameinf.bin";
    static constexpr std::size_t fileSize=0x24u;
    std::uint32_t routine=0x00413670u;
    std::uint32_t constructor=0x00423EE0u;
    // r225 DIRECT EXE 0x413C84..0x413C9D. EBP is zeroed at 0x41367F, so
    // Settings selects D3DCMP_ALWAYS, calls the shared 0x411D60 two-layer
    // background (which ends with 0x405E40), then restores LESSEQUAL.
    std::uint32_t backgroundRoutine=0x00411D60u;
    std::uint32_t backgroundFrameSubmit=0x00405E40u;
    std::uint32_t sceneRoutine=0x00412150u;
    int sceneTextureSlot=3;
    bool backgroundDepthAlways=true;
    bool backgroundDepthWriteEnabled=true;
    std::uint32_t speed=0x025B7878u,sfx=0x025B787Cu,music=0x025B7880u,speech=0x025B7898u;
    float sliderMin=0.f,sliderMax=1000.f,sliderRatePerMs=0.6000000238418579f;
    float defaultSpeed=800.f,defaultSfx=1000.f,defaultMusic=800.f;
    static constexpr int fadeMax=0x7C0;
    static constexpr int fadeInRate=7;
    static constexpr int fadeOutRate=-6;
    static constexpr int controlsReturnFadeInRate=6;
    // r350 DIRECT EXE 0x413B67..0x413BFA and 0x413CB0..0x413CD4.
    // The selected row owns a smoothed Z offset: target=-row*0.0028, moving
    // by 0.00005 per millisecond. The Settings camera then uses
    // z=selectorOffset*0.22-0.003 with fixed (0,-0x200,0) legacy angles.
    static constexpr float selectorRowStep=-0.00279999990016222f;
    static constexpr float selectorRatePerMs=4.999999873689376e-05f;
    static constexpr float cameraSelectorScale=0.2199999988079071f;
    static constexpr float cameraBaseZ=-0.003000000026077032f;
    static constexpr float cameraY=0.017999999225139618f;
    static constexpr int cameraAngle1=0, cameraAngle2=-0x200, cameraAngle3=0;
    static constexpr float selectorTarget(int row){ return float(row)*selectorRowStep; }
    static constexpr float cameraZ(float selectorOffset){ return selectorOffset*cameraSelectorScale+cameraBaseZ; }
    static constexpr float modelLightScale(int counter){
        const int c=counter<0?0:(counter>fadeMax?fadeMax:counter);
        return float(c>>3)/255.f;
    }
    static constexpr float sessionScale(float speedValue){return 1.f+(speedValue-800.f)*0.00025f;}
    static constexpr float audioScale(float value){return value<=0.f?0.f:0.25f+value*0.00075f;}
};
inline constexpr LegacySettingsTrace kLegacySettingsTrace{};

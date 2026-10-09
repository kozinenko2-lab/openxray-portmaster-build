#pragma once
#include <vector>
#include <array>
#include <cstdint>
#include <string>

enum class LegacyHudAtlas { Atlas3, Atlas4, Atlas7, MenuM1, MenuM2, Font5 };
enum class LegacyHudScreen { Gameplay, InterLevel, FinalSequence, MainMenu, ModeSelect, Records, Information, Settings, Controls, Complete, GameOver, Abort };

struct LegacyHudSprite {
    LegacyHudAtlas atlas=LegacyHudAtlas::Atlas3;
    float x=0.f,y=0.f,w=0.f,h=0.f;
    int sx=0,sy=0,sw=0,sh=0;
    float brightness=1.f;
    float r=1.f,g=1.f,b=1.f;
    // Normalized UV override for half-pixel legacy fnt4 glyph bounds.
    float u0=-1.f,v0=-1.f,u1=-1.f,v1=-1.f;
};

struct LegacyHudState {
    int lives=0;
    int timeRemaining=0;
    int levelNumber=1;
    int score=0;
    int capturePercent=0;
    int pulseCounter=0;
    bool paused=false;
    int menuSelected=0;
    int modeSelected=0;
    int modeAnglePhase=0;
    int modeFadeCounter=0;
    std::array<int,5> modeLevelCounts{{7,15,20,20,20}};
    std::array<std::string,5> modeNames{};
    std::array<float,5> menuBrightness{{1.f,1.f,1.f,1.f,1.f}};
    std::array<float,5> menuScale{{1.f,1.f,1.f,1.f,1.f}};
    int recordsMode=0;
    int recordsFadeCounter=0x7c0;
    std::string recordsModeName;
    // r238: exact working 200-byte high-score block copied by 0x40F0A0.
    // Names are fixed-width 16-byte CP1251 fields; values are uint32.
    std::array<std::array<char,16>,10> recordsNames=[](){
        std::array<std::array<char,16>,10> out{};
        for(auto& name:out)name.fill('.');
        return out;
    }();
    std::array<std::uint32_t,10> recordsValues{};
    bool recordsNameEntry=false;
    int recordsCandidateRow=-1;
    int recordsTypedNameLength=0;
    int recordsNameCursor=0;
    bool recordsDpadNameEditing=false;
    int recordsNamePulseByte=255;
    int recordsHeadingPhase=0;
    int informationPage=0;
    int informationFadeCounter=0x7c0;
    int informationPromptPhase=0;
    int settingsSelected=0;
    std::array<float,8> settingsBrightness{{1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f}};
    std::array<float,8> settingsScale{{1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f}};
    float settingsSpeed=800.f,settingsSfx=1000.f,settingsMusic=800.f;
    float settingsSelectorOffset=0.f;
    float settingsFadeScale=1.f;
    int testInitialLives=3,testInitialTimeSeconds=60;
    bool settingsSpeech=true;
    std::array<int,4> controlsBindings{{0x41,0x5A,0x58,0x43}};
    int controlsAssigned=0;
    bool controlsAwaitingConfirm=false;
    std::uint32_t controlsCurrentRowColor=0xcfcf40u;
    std::uint32_t controlsPromptColor=0x80af80u;
    float controlsFadeScale=1.f;
    int controlsFadeCounter=0x7c0;
    LegacyHudScreen screen=LegacyHudScreen::Gameplay;
};

namespace LegacyHud {
// The original renderer composes HUD sprites in a 640x480 design space.
// Atlas placements are confirmed from the x86 table. Screen coordinates are
// intentionally isolated here. Normal gameplay destinations are direct-EXE
// contracts; cinematic presentation meshes are handled by the world renderer.
std::vector<LegacyHudSprite> compose(const LegacyHudState& state);

// Exposed for deterministic tests and for future callsite-by-callsite RE.
void appendNumber(std::vector<LegacyHudSprite>& out,LegacyHudAtlas atlas,
                  int value,int minDigits,float x,float y,bool rightAlign=false);
}

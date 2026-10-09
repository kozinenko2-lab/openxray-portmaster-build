#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

// Keep the gameplay-facing input state independent from SDL. The native
// platform implementation includes SDL only in input.cpp, which lets the
// reconstructed game core be compiled/tested without any platform SDK.
struct _SDL_GameController;
struct _SDL_Joystick;

struct InputState {
    static constexpr std::size_t LegacyCodeCount=0x11C;
    bool quit = false;
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool action = false;
    bool back = false;
    bool pause = false;
    bool select = false; // PortMaster Back/Select; reserved for native diagnostics chords
    // Original AirXonix key namespace. 0x00..0xFF are Win32 VK-like codes;
    // 0x100..0x11B are J1/J2 directions/buttons from table 0x00440A40.
    std::array<bool,LegacyCodeCount> legacyHeld{};
    int legacyPressedCode = -1; // one edge event for modal remapping
    // r248: source tag prevents controller J1/D-Pad edge codes from being
    // mistaken for PC keyboard bytes by the post-game name editor.
    bool legacyPressedFromController = false;
    // r243: actual typed text byte converted from SDL UTF-8 to Windows-1251.
    // This is separate from legacyPressedCode because SDLK_a is a VK-like 'A'
    // regardless of Shift/case/layout, while 0x40F160 consumes typed bytes.
    int legacyTextByte = -1;
    bool legacyDown(int code) const {
        return code>=0 && static_cast<std::size_t>(code)<legacyHeld.size() && legacyHeld[static_cast<std::size_t>(code)];
    }
};

class InputSystem {
public:
    InputSystem();
    ~InputSystem();
    void poll(InputState& state);
    void setRecordNameTextInput(bool enabled); // Android: show IME ONLY in high-score name editor
    bool rumble(float strength01,std::uint32_t durationMs);
private:
    _SDL_GameController* controller_ = nullptr;
    _SDL_Joystick* joystick_ = nullptr;
    bool ownsJoystick_ = false;
    int lastLoggedHat_ = -1;
    int lastLoggedAxisX_ = 0x7fffffff;
    int lastLoggedAxisY_ = 0x7fffffff;
    unsigned loggedButtonMask_ = 0;
    int lastCardinalDirection_ = 0; // 1 up, 2 down, 3 left, 4 right
    unsigned lastRawHatState_ = 0;
    bool preferRawDirections_ = false; // muOS/Deeplay: raw hat/axes are more reliable than mapped GC directions

    void closeDevice();
    void openFirstInputDevice();
};

# AirXonix r370 — Android touchscreen keyboard codes
Стрелки: Up VK 0x26, Down VK 0x28, Left VK 0x25, Right VK 0x27.
A = Enter (0x0D), B = Escape (0x1B), pause = P (0x50).
Touch input no longer synthesizes J1 gamepad key codes.
ModeSelect uses held SDL keyboard semantics and release-to-arm rather than menu-repeat pulses.
Fast taps remain buffered; main menu keeps comfortable stick auto-repeat.
Physical gamepads and H700 PortMaster path unchanged.
Tests: native/tests/test_android_keyboard_r370.cpp.

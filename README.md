# AirXonix — Android r373, H700 unchanged

**Source archive:** [AirXonix-H700-Android-r373-public-source.zip](AirXonix-H700-Android-r373-public-source.zip)

This development branch shares the native game core between Linux H700/PortMaster and Android ARM64. **H700 input behavior is unchanged.**

Android r373 is a **diagnostic touchscreen-only release**. SDL virtual joystick and raw button events are entirely excluded from the Android input path, avoiding spurious A/Enter from joystick button zero at boot. An on-screen “TOUCH: hex-mask” in the upper-left verifies Android receives finger events. JNI also logs UI and GAME events via `adb logcat -s AirXonixTouch:D`.

Controls: analogue touch stick -> arrows, A -> Enter, B -> Escape; fast taps buffered. Android Bluetooth/USB gamepads are temporarily disabled in this diagnostic version. Name entry is the only place where Android's keyboard may appear.

GitHub Actions **Build AirXonix Android** builds an Android ARM64 clean-room-resource APK and runs startup/selector tests; the user-owned original EXE and MUSIC are **not published to this repository**. Full APK with the user's own resources is assembled privately.

Tests cannot replace a physical Android device test. Please report whether the TOUCH hex-mask changes when moving the stick or tapping A/B, and whether the game menu responds.

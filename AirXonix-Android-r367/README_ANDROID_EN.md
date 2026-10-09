# AirXonix r367 — self-contained Android ARM64 build project

Builds an installable APK with an Android C++ libmain.so, SDL2 2.30.12, OpenGL ES2, and packaged original user-supplied AirXonix.wrp.exe / MUSIC (11 tracks) plus fallback clean-room assets. The Windows EXE is a resource container; the native port executes instead of emulating Windows. App-private storage holds extracted assets, highscores and settings. No external folders or Internet are required when the APK is installed.

To build: use Android Studio / JDK 17 / Android SDK 35 / Android NDK / CMake 3.22.1 / Gradle 8.7; run `bash scripts/prepare_sdl2.sh` and `gradle :app:assembleDebug`. Result: `app/build/outputs/apk/debug/app-debug.apk`. Alternatively upload this project into your GitHub repo and run the included `.github/workflows/android-apk.yml` Actions workflow, then download the APK artifact.

Controls: multi-touch D-pad, A/B/pause, Bluetooth/USB controller; vibrator 82%/260ms; landscape 4:3 letterboxing. Android 8+ ARM64.

Status: Android APK compilation and physical device testing are NOT performed in this container (SDK/NDK/Gradle unavailable). This is a buildable-intent source project, not a verified APK.

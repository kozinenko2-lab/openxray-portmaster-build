# AirXonix r367 — H700 + Android source

- `native/`: H700 PortMaster ARM64 project and shared game logic.
- `app/`, `CMakeLists.txt`: native Android SDL2 project.
- `scripts/prepare_sdl2.sh`: clone matched SDL2 2.30.12 Java + C bridge.
- `scripts/bundle_personal_assets.py`: optional local-only original EXE/MUSIC injector (never commit).
- `.github/workflows/android-apk.yml`: public CI build with newly generated clean-room assets.

## Build Android locally with bundled original resources

Install Android SDK 35, NDK 27.2, CMake 3.22, Gradle 8.7/JDK 17.

```sh
bash scripts/prepare_sdl2.sh
python3 -m pip install numpy pillow
python3 native/tools/generate_cleanroom_resources.py /tmp/airxonix-assets
cp /tmp/AirXonix-cleanroom.zip app/src/main/assets/
python3 scripts/bundle_personal_assets.py /path/to/your/game
gradle :app:assembleDebug
```

The resulting APK embeds all assets supplied at build time. The public CI artifact uses **only generated, noncommercial** assets; it does not embed original commercial game material.

## Build H700

Use the H700 ARM64 sysroot and cross-compiler and follow `native/README_RU.md` + `native/scripts/ARM64_LINK_R367.md`. The H700 launcher is `native/portmaster/AirXonix.sh`.

**Status:** Android APK has not been compiler-validated locally because Android SDK/NDK are unavailable in the working environment. Device playtesting is still required.

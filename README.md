# AirXonix r369 — H700 / Android

This branch contains the H700 PortMaster + Android ARM64 source tree. OpenXRay main branch is unchanged.

Latest complete source: [AirXonix-H700-Android-r369-public-source.zip](AirXonix-H700-Android-r369-public-source.zip).

**Android r369:** launcher icon now uses the chosen first AIR XONIX illustration; Android adaptive and legacy icons; JNI buffers short A/B touch presses; joystick navigates menus with initial delay (340 ms) and repeat (145 ms); menu animation releases no longer swallow the next input; opening mode selector accepts first fresh input. Gameplay still uses uninterrupted cardinal movement.

- Build: GitHub Actions **Build AirXonix Android** creates an Android ARM64 debug APK with clean-room resources.
- Original game EXE and MUSIC are **not publicly included**; the user's personal APK can bundle their originals.
- Includes r369 unit/regression tests and common native game source.
- See `AirXonix-Android-r367/CHANGELOG_R369_RU.md` for the detailed changes (directory name preserved for build compatibility).

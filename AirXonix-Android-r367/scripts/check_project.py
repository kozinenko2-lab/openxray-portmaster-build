#!/usr/bin/env python3
"""Offline, dependency-free Android/H700 structure check."""
from pathlib import Path
import sys
root=Path(__file__).resolve().parents[1]
required=['native/src/main.cpp','native/src/platform/input.cpp','native/portmaster/AirXonix.sh','app/src/main/java/com/airxonix/nativeport/AirXonixActivity.java','app/build.gradle','CMakeLists.txt','scripts/prepare_sdl2.sh','native/tests/test_port_fixes_r367.cpp']
missing=[n for n in required if not (root/n).is_file()]
if missing: raise SystemExit('Missing: '+str(missing))
activity=(root/'app/src/main/java/com/airxonix/nativeport/AirXonixActivity.java').read_text()
assert 'OPTIONAL_FILES' in activity and 'REQUIRED_FILES' in activity
assert 'nativeSetPadMask' in activity and 'vibrateOnDeath' in activity
print('PASS: Android and H700 sources, Android input, asset manifest, launcher')

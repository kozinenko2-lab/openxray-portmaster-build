#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
bash scripts/prepare_sdl2.sh
if command -v gradle >/dev/null 2>&1; then
   gradle --no-daemon --console=plain assembleDebug
elif [ -x ./gradlew ]; then
   ./gradlew --no-daemon --console=plain assembleDebug
else
   echo 'Gradle not found. Open this project in Android Studio or install Gradle 8.7.' >&2
   exit 1
fi
printf '\nAPK: app/build/outputs/apk/debug/app-debug.apk\n'

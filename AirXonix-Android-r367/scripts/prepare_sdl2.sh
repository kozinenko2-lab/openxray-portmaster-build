#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
# SDL 2.30.12 is kept in lockstep with its JNI Java files.
if [ ! -e third_party/SDL/CMakeLists.txt ]; then
    git clone --depth 1 --branch release-2.30.12 https://github.com/libsdl-org/SDL.git third_party/SDL
fi
src=third_party/SDL/android-project/app/src/main/java/org/libsdl/app
if [ ! -d "$src" ]; then echo "Missing SDL2 Android Java sources" >&2; exit 1; fi
mkdir -p app/src/main/java/org/libsdl/app
cp -R "$src"/. app/src/main/java/org/libsdl/app/
echo 'SDL 2.30.12 Android JNI+Java prepared'

#!/usr/bin/env bash
set -euxo pipefail

export DEBIAN_FRONTEND=noninteractive

# ARM64 Ubuntu 18.04 packages are still published on the Ubuntu ports mirror.
cat >/etc/apt/sources.list <<'EOF'
deb http://ports.ubuntu.com/ubuntu-ports bionic main restricted universe multiverse
deb http://ports.ubuntu.com/ubuntu-ports bionic-updates main restricted universe multiverse
deb http://ports.ubuntu.com/ubuntu-ports bionic-security main restricted universe multiverse
deb http://ports.ubuntu.com/ubuntu-ports bionic-backports main restricted universe multiverse
EOF

apt-get update
apt-get install -y --no-install-recommends   ca-certificates curl git make ninja-build pkg-config   gcc-8 g++-8 binutils   autoconf automake libtool   libopenal-dev libjpeg-dev libogg-dev libvorbis-dev   libegl1-mesa-dev libgles2-mesa-dev libgl1-mesa-dev   libdrm-dev libgbm-dev libudev-dev   libasound2-dev libpulse-dev libdbus-1-dev   libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxinerama-dev   libwayland-dev libwayland-egl1-mesa   zlib1g-dev

ln -sf /usr/bin/gcc-8 /usr/local/bin/gcc
ln -sf /usr/bin/g++-8 /usr/local/bin/g++
export PATH=/usr/local/bin:$PATH
export CC=gcc-8
export CXX=g++-8

# CMake from Ubuntu 18.04 is too old for OpenXRay (requires >= 3.22).
CMAKE_VER=3.27.9
curl -fL --retry 3   "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VER}/cmake-${CMAKE_VER}-linux-aarch64.tar.gz"   -o /tmp/cmake.tar.gz
mkdir -p /opt/cmake
tar -xzf /tmp/cmake.tar.gz -C /opt/cmake --strip-components=1
export PATH=/opt/cmake/bin:$PATH
cmake --version

# OpenXRay requests SDL >= 2.0.18. Bionic only ships 2.0.8, so build a
# glibc-2.27-compatible SDL ourselves and use it both for linking and packaging.
SDL_VER=2.0.22
curl -fL --retry 3   "https://github.com/libsdl-org/SDL/releases/download/release-${SDL_VER}/SDL2-${SDL_VER}.tar.gz"   -o /tmp/SDL2.tar.gz
rm -rf /tmp/SDL2
mkdir -p /tmp/SDL2
tar -xzf /tmp/SDL2.tar.gz -C /tmp/SDL2 --strip-components=1
cmake -S /tmp/SDL2 -B /tmp/SDL2/build -G Ninja   -DCMAKE_BUILD_TYPE=Release   -DCMAKE_INSTALL_PREFIX=/opt/h700   -DSDL_SHARED=ON   -DSDL_STATIC=OFF   -DSDL_TEST=OFF   -DSDL_VULKAN=OFF
cmake --build /tmp/SDL2/build --parallel 4
cmake --install /tmp/SDL2/build

export CMAKE_PREFIX_PATH=/opt/h700
export PKG_CONFIG_PATH=/opt/h700/lib/pkgconfig:/opt/h700/lib/aarch64-linux-gnu/pkgconfig
export LD_LIBRARY_PATH=/opt/h700/lib:/opt/h700/lib/aarch64-linux-gnu

cd /workspace/openxray
rm -rf build-h700-glibc227
cmake -S . -B build-h700-glibc227 -G Ninja   -DCMAKE_BUILD_TYPE=Release   -DCMAKE_PREFIX_PATH=/opt/h700   -DSDL2_DIR=/opt/h700/lib/cmake/SDL2   -DXRAY_USE_GLES=ON   -DGLI_TEST_ENABLE=OFF   -DLUABIND_BUILD_TESTING=OFF   -DMEMORY_ALLOCATOR=standard   -DCMAKE_C_FLAGS="-march=armv8-a -mcpu=cortex-a53 -mtune=cortex-a53"   -DCMAKE_CXX_FLAGS="-march=armv8-a -mcpu=cortex-a53 -mtune=cortex-a53"   2>&1 | tee /workspace/logs/cmake-glibc227-configure.log

cmake --build build-h700-glibc227 --parallel 4   2>&1 | tee /workspace/logs/cmake-glibc227-build.log

OUT=/workspace/openxray/bin/aarch64/Release
test -x "$OUT/xr_3da"

file "$OUT/xr_3da" | tee /workspace/logs/xr_3da-glibc227-file.txt

# Verify every engine ELF stays within the H700 glibc baseline.
: > /workspace/logs/glibc-requirements.txt
bad=0
for f in "$OUT"/*; do
  if file "$f" | grep -q ELF; then
    maxv="$(readelf --version-info "$f" 2>/dev/null | grep -o 'GLIBC_[0-9.]*' | sort -Vu | tail -1 || true)"
    printf '%-26s %s\n' "$(basename "$f")" "$maxv" | tee -a /workspace/logs/glibc-requirements.txt
    if [ -n "$maxv" ]; then
      num="${maxv#GLIBC_}"
      if dpkg --compare-versions "$num" gt 2.27; then
        echo "ERROR: $(basename "$f") requires $maxv (> GLIBC_2.27)" | tee -a /workspace/logs/glibc-requirements.txt
        bad=1
      fi
    fi
  fi
done
test "$bad" -eq 0

# Save our compatible SDL runtime and compiler runtimes for staging.
mkdir -p /workspace/runtime-glibc227
cp -L /opt/h700/lib/libSDL2-2.0.so.0 /workspace/runtime-glibc227/
cp -L "$(gcc-8 -print-file-name=libgcc_s.so.1)" /workspace/runtime-glibc227/
cp -L "$(g++-8 -print-file-name=libstdc++.so.6)" /workspace/runtime-glibc227/

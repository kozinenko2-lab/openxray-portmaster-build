#!/usr/bin/env bash
set -euxo pipefail

cd /root/workspace/eka2l1
git config --global --add safe.directory /root/workspace/eka2l1

echo "Base H700 compiler:"
"$CC" --version
BASE_H700_SYSROOT="$SYSROOT"
echo "BASE_H700_SYSROOT=$BASE_H700_SYSROOT"
echo "PREFIX_LOCAL=$PREFIX_LOCAL"

mkdir -p /root/workspace/.cache /root/workspace/ci-artifacts

# Current Dynarmic/Oaknut requires a real C++20 standard library.
GCC11_ARCHIVE=/root/workspace/.cache/arm-gnu-toolchain-11.3.rel1-x86_64-aarch64-none-linux-gnu.tar.xz
GCC11_DIR=/opt/arm-gcc11
GCC11_URL="https://developer.arm.com/-/media/Files/downloads/gnu/11.3.rel1/binrel/arm-gnu-toolchain-11.3.rel1-x86_64-aarch64-none-linux-gnu.tar.xz"
GCC11_SHA256="50cdef6c5baddaa00f60502cc8b59cc11065306ae575ad2f51e412a9b2a90364"

rm -rf "$GCC11_DIR"
if [ ! -f "$GCC11_ARCHIVE" ]; then
  wget -q --show-progress -O "$GCC11_ARCHIVE" "$GCC11_URL"
fi
echo "$GCC11_SHA256  $GCC11_ARCHIVE" | sha256sum -c -
mkdir -p "$GCC11_DIR"
tar -xJf "$GCC11_ARCHIVE" -C "$GCC11_DIR" --strip-components=1

NEW_CC="$GCC11_DIR/bin/aarch64-none-linux-gnu-gcc"
NEW_CXX="$GCC11_DIR/bin/aarch64-none-linux-gnu-g++"
NEW_CC_ROOT="$GCC11_DIR/aarch64-none-linux-gnu"
READELF="$GCC11_DIR/bin/aarch64-none-linux-gnu-readelf"
GCC11_SYSROOT="$("$NEW_CXX" --print-sysroot)"

echo "Selected C++20 compiler:"
"$NEW_CXX" --version
echo "GCC11_SYSROOT=$GCC11_SYSROOT"

cat >/tmp/cxx20_probe.cpp <<'EOF'
#include <bit>
#include <compare>
#include <cstdint>
constexpr unsigned f(unsigned x) {
    return std::popcount(x) + std::countr_zero(x | 1u) + std::rotl(x, 3);
}
struct X { int v; auto operator<=>(const X&) const = default; };
int main() { X a{1}, b{2}; return (a < b && f(3u)) ? 0 : 1; }
EOF

"$NEW_CXX" --sysroot="$GCC11_SYSROOT" -std=c++20 -mcpu=cortex-a53 -mtune=cortex-a53 \
  /tmp/cxx20_probe.cpp -o /tmp/cxx20_probe

echo "=== C++20 probe version requirements ==="
"$READELF" -V /tmp/cxx20_probe | grep -E "GLIBC_|GLIBCXX_|CXXABI_" | tail -40 || true

# H700 userland target is glibc 2.35. Refuse to produce a binary that silently
# raises that floor.
if "$READELF" -V /tmp/cxx20_probe | grep -Eq "GLIBC_2\.(3[6-9]|[4-9][0-9])"; then
  echo "ERROR: C++20 probe requires glibc newer than 2.35" >&2
  exit 1
fi

LIBSTDCPP_REAL="$("$NEW_CXX" --print-file-name=libstdc++.so.6)"
LIBGCC_REAL="$("$NEW_CXX" --print-file-name=libgcc_s.so.1)"
echo "GCC11 libstdc++: $LIBSTDCPP_REAL"
"$READELF" -V "$LIBSTDCPP_REAL" | grep -E "GLIBC_[0-9]" | tail -30 || true

cp -L "$LIBSTDCPP_REAL" /root/workspace/ci-artifacts/libstdc++.so.6
cp -L "$LIBGCC_REAL" /root/workspace/ci-artifacts/libgcc_s.so.1


# FFmpeg is configured outside the CMake target graph and therefore cannot use
# EKA2L1's zlibstatic target directly. Reuse the H700 SDK zlib through a tiny
# isolated overlay so old glibc headers never shadow the GCC11/glibc-2.35 sysroot.
H700_ZLIB_OVERLAY=/opt/h700-zlib-overlay
rm -rf "$H700_ZLIB_OVERLAY"
mkdir -p "$H700_ZLIB_OVERLAY/include" "$H700_ZLIB_OVERLAY/lib"

ZLIB_HEADER="$(find "$BASE_H700_SYSROOT" -name zlib.h -print -quit 2>/dev/null || true)"
ZLIB_STATIC="$(find "$BASE_H700_SYSROOT" -name libz.a -print -quit 2>/dev/null || true)"
ZLIB_SHARED="$(find "$BASE_H700_SYSROOT" \( -name libz.so -o -name 'libz.so.*' \) -print -quit 2>/dev/null || true)"
ZLIB_LIB="$ZLIB_STATIC"
if [ -z "$ZLIB_LIB" ]; then
  ZLIB_LIB="$ZLIB_SHARED"
fi

FFMPEG_ZLIB_CFLAGS=
FFMPEG_ZLIB_LDFLAGS=
if [ -n "$ZLIB_HEADER" ] && [ -n "$ZLIB_LIB" ]; then
  ZLIB_INCLUDE_DIR="$(dirname "$ZLIB_HEADER")"
  cp -L "$ZLIB_HEADER" "$H700_ZLIB_OVERLAY/include/zlib.h"
  if [ -f "$ZLIB_INCLUDE_DIR/zconf.h" ]; then
    cp -L "$ZLIB_INCLUDE_DIR/zconf.h" "$H700_ZLIB_OVERLAY/include/zconf.h"
  fi

  if [ -n "$ZLIB_STATIC" ]; then
    cp -L "$ZLIB_STATIC" "$H700_ZLIB_OVERLAY/lib/libz.a"
    echo "FFmpeg zlib: static $ZLIB_STATIC"
  else
    cp -L "$ZLIB_SHARED" "$H700_ZLIB_OVERLAY/lib/libz.so"
    echo "FFmpeg zlib: shared $ZLIB_SHARED"
  fi

  FFMPEG_ZLIB_CFLAGS="-I$H700_ZLIB_OVERLAY/include"
  FFMPEG_ZLIB_LDFLAGS="-L$H700_ZLIB_OVERLAY/lib"

  # Final link must not expose the complete old glibc-2.33 SDK to ld:
  # doing so can make ld pick its libpthread/libdl and then fail on GLIBC_PRIVATE.
  # Copy only the ABI-compatible DT_NEEDED libraries required by H700 SDL2/libpng
  # into an isolated overlay. Their libc/libm dependencies are then resolved from
  # the GCC11/glibc-2.35 sysroot.
  H700_RUNTIME_OVERLAY=/opt/h700-runtime-overlay
  rm -rf "$H700_RUNTIME_OVERLAY"
  mkdir -p "$H700_RUNTIME_OVERLAY/lib"

  SAMPLERATE_SHARED="$(find "$BASE_H700_SYSROOT" -name 'libsamplerate.so.0' -print -quit 2>/dev/null || true)"
  if [ -z "$SAMPLERATE_SHARED" ]; then
    SAMPLERATE_SHARED="$(find "$BASE_H700_SYSROOT" -name 'libsamplerate.so.0.*' -print -quit 2>/dev/null || true)"
  fi
  ZLIB_SONAME="$(find "$BASE_H700_SYSROOT" -name 'libz.so.1' -print -quit 2>/dev/null || true)"
  if [ -z "$ZLIB_SONAME" ]; then
    ZLIB_SONAME="$(find "$BASE_H700_SYSROOT" -name 'libz.so.1.*' -print -quit 2>/dev/null || true)"
  fi

  if [ -z "$SAMPLERATE_SHARED" ] || [ -z "$ZLIB_SONAME" ]; then
    echo "ERROR: required H700 runtime libs not found (libsamplerate.so.0/libz.so.1)" >&2
    find "$BASE_H700_SYSROOT" \
      \( -name 'libsamplerate.so*' -o -name 'libz.so*' \) -print 2>/dev/null | head -80 || true
    exit 1
  fi

  cp -L "$SAMPLERATE_SHARED" "$H700_RUNTIME_OVERLAY/lib/libsamplerate.so.0"
  cp -L "$ZLIB_SONAME" "$H700_RUNTIME_OVERLAY/lib/libz.so.1"
  H700_RPATH_LINKS="-Wl,-rpath-link,$PREFIX_LOCAL/lib -Wl,-rpath-link,$H700_RUNTIME_OVERLAY/lib"

  echo "=== isolated H700 runtime overlay ==="
  ls -l "$H700_RUNTIME_OVERLAY/lib"
  "$READELF" -d "$H700_RUNTIME_OVERLAY/lib/libsamplerate.so.0" | grep NEEDED || true
  "$READELF" -d "$H700_RUNTIME_OVERLAY/lib/libz.so.1" | grep NEEDED || true
else
  echo "ERROR: H700 SDK zlib headers/library not found under $BASE_H700_SYSROOT" >&2
  find "$BASE_H700_SYSROOT" -maxdepth 5 \( -name zlib.h -o -name libz.a -o -name 'libz.so*' \) -print 2>/dev/null | head -50 || true
  exit 1
fi

cat >/tmp/eka2l1-gcc11-h700.cmake <<EOF
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_C_COMPILER "$NEW_CC")
set(CMAKE_CXX_COMPILER "$NEW_CXX")
set(CMAKE_SYSROOT "$GCC11_SYSROOT")
list(APPEND CMAKE_FIND_ROOT_PATH "$PREFIX_LOCAL" "$GCC11_SYSROOT" "$BASE_H700_SYSROOT" "$NEW_CC_ROOT")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
EOF

rm -rf build-h700
mkdir -p build-h700

cmake -S . -B build-h700 -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_TOOLCHAIN_FILE=/tmp/eka2l1-gcc11-h700.cmake \
  -DCMAKE_PREFIX_PATH="$PREFIX_LOCAL" \
  -DCMAKE_C_FLAGS="-mcpu=cortex-a53 -mtune=cortex-a53 $FFMPEG_ZLIB_CFLAGS" \
  -DCMAKE_CXX_FLAGS="-mcpu=cortex-a53 -mtune=cortex-a53" \
  -DCMAKE_EXE_LINKER_FLAGS="$FFMPEG_ZLIB_LDFLAGS $H700_RPATH_LINKS" \
  -DEKA2L1_PORTMASTER=ON \
  -DEKA2L1_BUILD_TESTS=OFF \
  -DEKA2L1_BUILD_TOOLS=OFF \
  -DEKA2L1_BUILD_PATCH=OFF \
  -DEKA2L1_ENABLE_DISCORD_RICH_PRESENCE=OFF \
  2>&1 | tee build-h700/configure.log

cmake --build build-h700 --target eka2l1_portmaster -j2   2>&1 | tee build-h700/build.log

file build-h700/bin/eka2l1_portmaster
"$READELF" -h build-h700/bin/eka2l1_portmaster
"$READELF" -d build-h700/bin/eka2l1_portmaster

cp -f build-h700/bin/eka2l1_portmaster /root/workspace/ci-artifacts/

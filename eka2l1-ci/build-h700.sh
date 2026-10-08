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

"$NEW_CXX" --sysroot="$SYSROOT" -std=c++20 -mcpu=cortex-a53 -mtune=cortex-a53   /tmp/cxx20_probe.cpp -o /tmp/cxx20_probe

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

cmake -S . -B build-h700 -G Ninja   -DCMAKE_BUILD_TYPE=RelWithDebInfo   -DCMAKE_TOOLCHAIN_FILE=/tmp/eka2l1-gcc11-h700.cmake   -DCMAKE_PREFIX_PATH="$PREFIX_LOCAL"   -DCMAKE_C_FLAGS="-mcpu=cortex-a53 -mtune=cortex-a53"   -DCMAKE_CXX_FLAGS="-mcpu=cortex-a53 -mtune=cortex-a53"   -DEKA2L1_PORTMASTER=ON   -DEKA2L1_BUILD_TESTS=OFF   -DEKA2L1_BUILD_TOOLS=OFF   -DEKA2L1_BUILD_PATCH=OFF   -DEKA2L1_ENABLE_DISCORD_RICH_PRESENCE=OFF   2>&1 | tee build-h700/configure.log

cmake --build build-h700 --target eka2l1_portmaster -j2   2>&1 | tee build-h700/build.log

file build-h700/bin/eka2l1_portmaster
"$READELF" -h build-h700/bin/eka2l1_portmaster
"$READELF" -d build-h700/bin/eka2l1_portmaster

cp -f build-h700/bin/eka2l1_portmaster /root/workspace/ci-artifacts/

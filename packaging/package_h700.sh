#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_REPO="$(cd "$SCRIPT_DIR/.." && pwd)"

if [ "$#" -lt 4 ]; then
  echo "usage: $0 <openxray-source> <bin-dir> <runtime-dir> <output-dir>" >&2
  exit 2
fi

SRC="$(cd "$1" && pwd)"
BIN="$(cd "$2" && pwd)"
RUNTIME="$(cd "$3" && pwd)"
OUT="$4"
STAGE="$OUT/Stalker"

rm -rf "$OUT"
mkdir -p "$STAGE/libs.aarch64"

test -x "$BIN/xr_3da"
cp "$BIN/xr_3da" "$STAGE/xr_3da.aarch64"
chmod +x "$STAGE/xr_3da.aarch64"

for so in "$BIN"/*.so; do
  [ -e "$so" ] || continue
  cp -L "$so" "$STAGE/libs.aarch64/"
done

for so in "$RUNTIME"/*.so*; do
  [ -e "$so" ] || continue
  cp -L "$so" "$STAGE/libs.aarch64/$(basename "$so")"
done

# Theora and LZO are statically linked in the H700 build.
rm -f "$STAGE/libs.aarch64"/libtheora*.so* "$STAGE/libs.aarch64"/liblzo*.so*

cp "$SRC/res/fsgame.ltx" "$STAGE/fsgame.ltx"
rm -rf "$STAGE/gamedata"
cp -a "$SRC/res/gamedata" "$STAGE/gamedata"

# H700 uses only the OpenGL/GLES renderer. Do not ship the desktop DirectX
# shader trees (r1/r2/r3). Rebuild the GL overlay with Linux case-sensitive
# include paths verified.
rm -rf "$STAGE/gamedata/shaders/r1" \
       "$STAGE/gamedata/shaders/r2" \
       "$STAGE/gamedata/shaders/r3"
python3 "$BUILD_REPO/h700/prepare_shader_overlay.py" \
  "$SRC/res/gamedata/shaders/gl" \
  "$STAGE/gamedata/shaders/gl" \
  | tee "$STAGE/SHADER_OVERLAY.txt"

cp "$SRC/portmaster/ports/openxray/OpenXRay.sh" "$STAGE/OpenXRay.sh"
chmod +x "$STAGE/OpenXRay.sh"
sed -i 's|log\.txt|Stalker.log|g' "$STAGE/OpenXRay.sh"

cp "$SRC/portmaster/ports/openxray/stalker.sh" "$OUT/stalker.sh"
chmod +x "$OUT/stalker.sh"

cat >"$STAGE/README_RU.txt" <<'EOF'
S.T.A.L.K.E.R. OpenXRay — H700 / ARM64 / PortMaster

Игровые данные не входят в пакет.

Для Shadow of Chernobyl скопируйте оригинальные архивы:
  gamedata.db0 ... gamedata.dbd
в эту папку Stalker, рядом с fsgame.ltx.

MuOS:
  /mnt/sdcard/roms/Ports/stalker.sh
  /mnt/sdcard/roms/Ports/Stalker/

Лог запуска:
  Stalker/Stalker.log
EOF

cat >"$STAGE/PORT_BUILD_INFO.txt" <<EOF
Target: Anbernic RG40XX H / Allwinner H700
Architecture: ARM64 / Cortex-A53
Renderer: OpenGL ES
SDL: SDL2
ABI baseline: glibc <= 2.27
Game archives: not included
EOF

{
  echo "=== ELF FILES ==="
  while IFS= read -r -d '' f; do
    if file "$f" | grep -q ELF; then
      echo
      echo "[$(basename "$f")]"
      file "$f"
      readelf -d "$f" 2>/dev/null | sed -n 's/.*Shared library: \[\(.*\)\]/NEEDED \1/p'
      maxv="$(readelf --version-info "$f" 2>/dev/null | grep -o 'GLIBC_[0-9.]*' | sort -Vu | tail -1 || true)"
      echo "MAX_GLIBC \${maxv:-none}"
    fi
  done < <(find "$STAGE" -type f -print0)
} > "$STAGE/ELF_DEPENDENCIES.txt"

bad=0
while IFS= read -r -d '' f; do
  file "$f" | grep -q ELF || continue
  maxv="$(readelf --version-info "$f" 2>/dev/null | grep -o 'GLIBC_[0-9.]*' | sort -Vu | tail -1 || true)"
  [ -n "$maxv" ] || continue
  v="\${maxv#GLIBC_}"
  if dpkg --compare-versions "$v" gt 2.27; then
    echo "ERROR: $f requires $maxv" >&2
    bad=1
  fi
done < <(find "$STAGE" -type f -print0)
test "$bad" -eq 0

(
  cd "$OUT"
  rm -f Stalker-H700-PortMaster.zip
  zip -qr Stalker-H700-PortMaster.zip stalker.sh Stalker
)

echo "Package: $OUT/Stalker-H700-PortMaster.zip"
du -h "$OUT/Stalker-H700-PortMaster.zip"

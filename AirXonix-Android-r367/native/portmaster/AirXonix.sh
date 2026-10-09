#!/bin/bash
# AirXonix FINAL — standalone PortMaster ARM64 launcher
# Target: AArch64 Cortex-A53 class systems (RK3326 / Allwinner H700)

PORT_NAME="AirXonix"
SCRIPT_DIR="$(cd "$(dirname "$0")" 2>/dev/null && pwd)"
PORTS_ROOT=""
PORT_DIR=""

# Controller vibration on life loss. Edit these three values if desired.
# ENABLED: 1=on, 0=off. DURATION is milliseconds. STRENGTH is 0..100 percent.
VIBRATION_ENABLED="${VIBRATION_ENABLED:-1}"
VIBRATION_DURATION_MS="${VIBRATION_DURATION_MS:-260}"
VIBRATION_STRENGTH="${VIBRATION_STRENGTH:-82}"
export AIRXONIX_VIBRATION_ENABLED="$VIBRATION_ENABLED"
export AIRXONIX_VIBRATION_DURATION_MS="$VIBRATION_DURATION_MS"
export AIRXONIX_VIBRATION_STRENGTH="$VIBRATION_STRENGTH"

# IMPORTANT: explicit firmware search priority requested for this port.
# muOS SD/MMC layouts must win before ArkOS /roms layouts.
for root in \
  "/mnt/sdcard/ports" \
  "/mnt/mmc/ports" \
  "/roms/ports" \
  "/roms2/ports" \
  "/storage/roms/ports" \
  "/userdata/roms/ports"; do
  if [ -d "$root/$PORT_NAME" ]; then
    PORTS_ROOT="$root"
    PORT_DIR="$root/$PORT_NAME"
    break
  fi
done

# Last-resort portable layout: launcher next to AirXonix/.
# This is deliberately checked only after all firmware port roots above.
if [ -z "$PORT_DIR" ] && [ -d "$SCRIPT_DIR/$PORT_NAME" ]; then
  PORTS_ROOT="$SCRIPT_DIR"
  PORT_DIR="$SCRIPT_DIR/$PORT_NAME"
fi

if [ -z "$PORT_DIR" ]; then
  echo "ERROR: AirXonix port directory not found."
  echo "Searched, in order:"
  echo "  /mnt/sdcard/ports/AirXonix"
  echo "  /mnt/mmc/ports/AirXonix"
  echo "  /roms/ports/AirXonix"
  echo "  /roms2/ports/AirXonix"
  echo "  /storage/roms/ports/AirXonix"
  echo "  /userdata/roms/ports/AirXonix"
  echo "  $SCRIPT_DIR/AirXonix"
  exit 127
fi

BIN="$PORT_DIR/airxonix"
LOG_DIR="$PORT_DIR/logs"
SAVE_DIR="$PORT_DIR/saves"
mkdir -p "$LOG_DIR" "$SAVE_DIR"

STAMP="$(date +%Y%m%d-%H%M%S 2>/dev/null || echo boot)"
LOG="$LOG_DIR/airxonix-$STAMP.log"
LAST="$LOG_DIR/airxonix-last.log"
: > "$LAST"
exec > >(tee -a "$LOG" "$LAST") 2>&1

echo "=== AirXonix FINAL standalone PortMaster ARM64 ==="
echo "Date: $(date 2>/dev/null || true)"
echo "SCRIPT_DIR=$SCRIPT_DIR"
echo "PORTS_ROOT=$PORTS_ROOT"
echo "PORT_DIR=$PORT_DIR"
echo "uname: $(uname -a 2>/dev/null || true)"

ARCH="$(uname -m 2>/dev/null || echo unknown)"
case "$ARCH" in
  aarch64|arm64) ;;
  *)
    echo "ERROR: This package requires AArch64/ARM64; detected: $ARCH"
    exit 126
    ;;
esac

# PortMaster control environment. Keep game-folder discovery above independent
# from control.txt discovery because firmware layouts differ here.
CONTROL=""
for c in \
  "$PORTS_ROOT/PortMaster/control.txt" \
  "/mnt/mmc/MUOS/PortMaster/control.txt" \
  "/mnt/sdcard/MUOS/PortMaster/control.txt" \
  "/roms/ports/PortMaster/control.txt" \
  "/roms2/ports/PortMaster/control.txt" \
  "/storage/roms/ports/PortMaster/control.txt" \
  "/userdata/roms/ports/PortMaster/control.txt" \
  "$SCRIPT_DIR/PortMaster/control.txt"; do
  if [ -f "$c" ]; then
    CONTROL="$c"
    break
  fi
done

if [ -n "$CONTROL" ]; then
  echo "PortMaster control: $CONTROL"
  # shellcheck disable=SC1090
  source "$CONTROL"
  if declare -F get_controls >/dev/null 2>&1; then
    get_controls >/dev/null 2>&1 || true
  fi
else
  echo "WARN: PortMaster control.txt not found; continuing with SDL defaults."
fi

# Native SDL controller access is intentional. Do not launch gptokeyb: it can
# seize joystick 0 before AirXonix opens it on some firmwares.
export HOME="$SAVE_DIR"
export XDG_DATA_HOME="$SAVE_DIR"
export SDL_VIDEO_MINIMIZE_ON_FOCUS_LOSS=0
export SDL_GAMECONTROLLER_USE_BUTTON_LABELS=0
[ -n "${sdl_controllerconfig:-}" ] && export SDL_GAMECONTROLLERCONFIG="$sdl_controllerconfig"
unset SDL_VIDEODRIVER SDL_AUDIODRIVER

if declare -F pm_platform_helper >/dev/null 2>&1; then
  pm_platform_helper >/dev/null 2>&1 || true
fi

if [ ! -x "$BIN" ]; then
  echo "ERROR: executable is missing or not executable: $BIN"
  ls -la "$PORT_DIR" 2>/dev/null || true
  exit 127
fi

cd "$PORT_DIR" || exit 127

echo "Binary: $(file "$BIN" 2>/dev/null || true)"
echo "Dependencies:"
ldd "$BIN" 2>&1 || true

echo "SDL_GAMECONTROLLERCONFIG=${SDL_GAMECONTROLLERCONFIG:-<unset>}"
echo "Vibration: enabled=$AIRXONIX_VIBRATION_ENABLED duration=${AIRXONIX_VIBRATION_DURATION_MS}ms strength=${AIRXONIX_VIBRATION_STRENGTH}%"
echo "Starting game..."
"$BIN"
RC=$?
echo "AirXonix exited with code $RC"

if [ "$RC" -ne 0 ]; then
  echo "--- recent kernel messages (if permitted) ---"
  dmesg 2>/dev/null | tail -80 || true
fi

exit "$RC"

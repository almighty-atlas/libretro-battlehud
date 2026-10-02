#!/bin/sh
# Isolated NextUI H700 emulator tag PBH; use the firmware's original Gambatte.
set -eu
fail() { printf 'battlehud: %s\n' "$*" >&2; exit 1; }
[ "$#" -eq 1 ] || fail 'Expected one ROM path'
[ -f "$1" ] || fail 'ROM not found'
for name in CORES_PATH BIOS_PATH SAVES_PATH CHEATS_PATH LOGS_PATH USERDATA_PATH; do
    eval 'value=${'"$name"':-}'
    [ -n "$value" ] || fail "NextUI environment missing $name"
done
case "${PLATFORM:-h700}" in h700) ;; *) fail 'This package targets H700' ;; esac
PAK_DIR=$(CDPATH= cd -P "$(dirname "$0")" && pwd)
WRAPPER="$PAK_DIR/gambatte_libretro.so"
BACKEND="$CORES_PATH/gambatte_libretro.so"
[ -f "$WRAPPER" ] || fail 'Packaged wrapper missing'
[ -f "$BACKEND" ] || fail 'Original firmware Gambatte missing'
command -v minarch.elf >/dev/null 2>&1 || fail 'MinArch missing from PATH'
mkdir -p "$BIOS_PATH/PBH" "$SAVES_PATH/PBH" "$CHEATS_PATH/PBH" "$LOGS_PATH"
cd "$USERDATA_PATH"
export LIBRETRO_BATTLEHUD_BACKEND="$BACKEND"
export LIBRETRO_BATTLEHUD_DEBUG="${LIBRETRO_BATTLEHUD_DEBUG:-1}"
exec minarch.elf "$WRAPPER" "$1" > "$LOGS_PATH/PBH.txt" 2>&1

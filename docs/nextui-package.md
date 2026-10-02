# BattleHUD NextUI H700 test package (M5)

Target: Anbernic RG SP on **pvaibhav/NextUI h700-rc11**, source commit
`cd73cd85c08449d939b7bfcb2e1ed3ed721f1dfa`. This is an isolated test package;
physical-device acceptance is pending. Other firmware releases need revalidation.

## Install without changing the existing GBC emulator

1. Extract `battlehud-nextui-h700.zip` at the root of the NextUI SD card. The wrapper
   and launcher land in `Emus/h700/PBH.pak/`. If downloading an Actions artifact,
   first extract its outer ZIP to obtain this package ZIP.
2. Create `Roms/Pokemon BattleHUD (PBH)/` and copy the supported, extracted Crystal
   `.gbc` into it. SHA-1 must be `f2f52230b536214ef7c9924f483392993e226cfb`.
3. Start Crystal from the new Pokemon BattleHUD ROM entry in NextUI.

No firmware files or existing GBC Pak are replaced. The wrapper is named
`gambatte_libretro.so` so MinArch keeps the core name `gambatte`. It dynamically
loads the firmware's original `$CORES_PATH/gambatte_libretro.so` by explicit
backend environment path. The original emulator core is not bundled or copied.
ROMs are not bundled. The launcher uses the existing NextUI environment and
MinArch executable, with working directory USERDATA_PATH.

The custom PBH tag isolates SRAM in `Saves/PBH/`, options in the platform userdata
`PBH-gambatte` directory and states in shared userdata `PBH-gambatte`. Existing
GBC SRAM/states are not automatically migrated. For the first test, start with a
separate test save; importing real saves is a later, deliberate step. BIOS and
cheat directories also use PBH. The standard GBC ROM entry still uses the original
core. Going back to it requires no rollback of firmware binaries.

Debug output goes to `$LOGS_PATH/PBH.txt`. The package manifest records the wrapper
commit, binary SHA-256 and selected firmware baseline. The outer CI artifact also
contains a SHA-256 file for the package ZIP. ZIP executable modes are recorded;
on an SD filesystem that preserves Unix permissions, launch.sh must remain
executable (mode 755).

## Device acceptance (still required)

Check normal startup, sound/input, main-menu and FIGHT badges, bag/party hiding,
opponent change, battle-end removal, save-state restore, SRAM quit/relaunch and
clean exit. Check suspend/resume on the real SP as a separate device behavior.
Inspect the PBH log for profile recognition and core loading. A successful cross
build and launcher mock do not establish physical device behavior.

HUD badges are enabled by default. The launcher enables debug logging unless
LIBRETRO_BATTLEHUD_DEBUG is explicitly 0. For a plain-emulation comparison,
LIBRETRO_BATTLEHUD_DISABLE_HUD=1 disables badges without changing the backend.

To stop using the package, use the normal GBC ROM entry. After the emulator has
closed, the custom PBH Pak/ROM entry can be removed; retain PBH save/state files if
needed. Nothing needs replacing in .system.

## Source-backed package layout

The selected fork's
[utils.c](https://github.com/pvaibhav/NextUI/blob/cd73cd85c08449d939b7bfcb2e1ed3ed721f1dfa/workspace/all/common/utils.c)
extracts the tag in the ROM folder's final parentheses and checks custom
`Emus/<platform>/<tag>.pak/launch.sh` before built-in Paks.
[nextui.c](https://github.com/pvaibhav/NextUI/blob/cd73cd85c08449d939b7bfcb2e1ed3ed721f1dfa/workspace/all/nextui/nextui.c)
recognizes those custom emulator paths.
[ma_core.c](https://github.com/pvaibhav/NextUI/blob/cd73cd85c08449d939b7bfcb2e1ed3ed721f1dfa/workspace/all/minarch/ma_core.c)
derives settings/state names from ROM tag and core filename, and SRAM directories
from the tag. The source review and device validation are recorded separately.

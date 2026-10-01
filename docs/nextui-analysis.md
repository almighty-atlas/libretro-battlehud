# NextUI / MinArch integration findings

Source inspected on 2026-10-01: [LoveRetro/NextUI](https://github.com/LoveRetro/NextUI),
commit `a0628cdc0cee8e173a9bb94144f5c8baa22ab8e7`. These are source-level findings,
not a successful device test.

## Core loading and API

[`workspace/all/minarch/ma_core.c`](https://github.com/LoveRetro/NextUI/blob/a0628cdc0cee8e173a9bb94144f5c8baa22ab8e7/workspace/all/minarch/ma_core.c)
loads a shared core using `dlopen(core_path, RTLD_LAZY)` and resolves the standard
`retro_*` entry points with `dlsym`. It requests system info before registering the
six environment/video/audio/input callbacks, then initializes and loads the game.
The wrapper supports this order; obtaining system info can load the backend before
any callbacks exist.

[`skeleton/SYSTEM/tg5040/paks/Emus/GBC.pak/launch.sh`](https://github.com/LoveRetro/NextUI/blob/a0628cdc0cee8e173a9bb94144f5c8baa22ab8e7/skeleton/SYSTEM/tg5040/paks/Emus/GBC.pak/launch.sh)
sets `EMU_EXE=gambatte` and starts `minarch.elf` with
`$CORES_PATH/gambatte_libretro.so` and the selected ROM. It does not start RetroArch.
This supports the proxy-core architecture without a frontend fork or daemon.

## Options, video and saves

[`ma_environment.c`](https://github.com/LoveRetro/NextUI/blob/a0628cdc0cee8e173a9bb94144f5c8baa22ab8e7/workspace/all/minarch/ma_environment.c)
handles software pixel formats, legacy and modern core options, and memory maps.
M0 forwards their command/data pointers and frontend return values unchanged.
Frontend OpenGL rendering does not require the Gambatte core itself to provide
hardware-rendered frames.

[`ma_saves.c`](https://github.com/LoveRetro/NextUI/blob/a0628cdc0cee8e173a9bb94144f5c8baa22ab8e7/workspace/all/minarch/ma_saves.c)
reads/writes SRAM and RTC through the core memory APIs and uses core serialization
for save states. `Core_quit` saves memory, unloads the game and deinitializes it;
`Core_close` closes the shared-library handle separately.

Important packaging detail: `Core_getName` derives the core name from the **filename**.
`gambatte_libretro.so` produces `gambatte`; the current development artifact
`pokemon_gambatte_libretro.so` produces `pokemon_gambatte`. Core options and save-state
directories include that name, so changing the filename can hide existing settings
and states even when the backend format stays compatible.

M5 should preserve the frontend-facing filename where continuity is intended:
install the wrapper as `gambatte_libretro.so` and the matching original core beside
it as `gambatte_real_libretro.so`. Do this in an isolated test package first. Preserve
the original core for rollback and verify state/SRAM continuity before replacement.
Both libraries must match the device architecture and userspace ABI. Package-specific
paths belong outside the portable runtime.

## Device scope and outstanding validation

The inspected [README](https://github.com/LoveRetro/NextUI/blob/a0628cdc0cee8e173a9bb94144f5c8baa22ab8e7/README.md)
lists TrimUI Brick, Smart Pro and Smart Pro S. It does **not** establish Anbernic SP
support. Do not label an Anbernic package as validated NextUI support without the
exact installed firmware/fork and its launcher/toolchain being identified.

The next M0 gate is a target-frontend run: start a GB/GBC game, check video/audio/input,
change a Gambatte option, save/reload SRAM and a save state, and quit/relaunch cleanly.
Then repeat under RetroArch. Automated headless Gambatte parity complements these
checks; it does not replace them. M1 starts after this gate is satisfied.

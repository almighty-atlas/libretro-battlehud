# NextUI / MinArch integration findings

Primary device target: [pvaibhav/NextUI h700-rc11](https://github.com/pvaibhav/NextUI/releases/tag/h700-rc11),
release dated 2026-09-27, commit `cd73cd85c08449d939b7bfcb2e1ed3ed721f1dfa`.
This is the firmware fork selected for the user's Anbernic SP. Its `main` branch
still carries the upstream TrimUI README; inspect the **H700 release tag**, not
`main`, for device support and packaging.

Original upstream source inspected on 2026-10-01: [LoveRetro/NextUI](https://github.com/LoveRetro/NextUI),
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

The selected H700 fork explicitly supports Anbernic H700 and mentions RG SP in
its release notes. Release rc11 is marked a **pre-release for Pak authors**; rc10
is the preceding release. Both release notes describe MinArch as the emulator
frontend. The target is now identified, but a successful physical-device test is
still outstanding.

At tag `h700-rc11`, the H700
[GBC launcher](https://github.com/pvaibhav/NextUI/blob/cd73cd85c08449d939b7bfcb2e1ed3ed721f1dfa/skeleton/SYSTEM/h700/paks/Emus/GBC.pak/launch.sh)
sets `EMU_EXE=gambatte` and invokes `minarch.elf` with
`$CORES_PATH/gambatte_libretro.so`. The inspected
[core loader](https://github.com/pvaibhav/NextUI/blob/cd73cd85c08449d939b7bfcb2e1ed3ed721f1dfa/workspace/all/minarch/ma_core.c)
has the same filename-derived settings/state paths and callback setup described
above. No change to the portable proxy architecture is needed.

The rc11 SDL button-layout change affects SDL-based Paks; its release notes
explicitly say MinArch reads input directly and is unaffected. BattleHUD receives
Libretro input callbacks from MinArch and therefore does not need an SDL button
mapping. Device input still needs a practical check.

## H700 build target

The fork uses `ghcr.io/loveretro/h700-toolchain:latest`, configured by
[makefile.toolchain](https://github.com/pvaibhav/NextUI/blob/cd73cd85c08449d939b7bfcb2e1ed3ed721f1dfa/makefile.toolchain).
The [toolchain Dockerfile](https://github.com/LoveRetro/h700-toolchain/blob/main/Dockerfile)
sets `CROSS_TRIPLE=aarch64-nextui-linux-gnu`, a GCC 8.3 cross compiler, and
`CMAKE_TOOLCHAIN_FILE=/opt/aarch64-nextui-linux-gnu/Toolchain.cmake`.
Its SDK sysroot uses glibc 2.33; the Dockerfile describes H700 stock userspace as
glibc 2.35. Do not substitute a current desktop Linux compiler's sysroot for it.
The H700 [platform flags](https://github.com/pvaibhav/NextUI/blob/cd73cd85c08449d939b7bfcb2e1ed3ed721f1dfa/workspace/h700/platform/makefile.env)
target Cortex-A53.

CI builds the M0 wrapper with that toolchain, disables native host tests in the
cross build, verifies ELF64/AArch64/shared-object headers, and uploads the
`battlehud-h700-m0` artifact. This is a test binary, not an installer or completed
M5 package. The image currently follows upstream's mutable `latest` tag; pin its
digest before publishing a reproducible release.

To build with Docker from the repository root:

```sh
docker run --rm -v "$PWD:/root/workspace" -w /root/workspace \
  ghcr.io/loveretro/h700-toolchain:latest sh -c '
    cmake -S . -B build-h700 \
      -DCMAKE_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN_FILE" \
      -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS=-mcpu=cortex-a53 \
      -DBUILD_TESTING=OFF &&
    cmake --build build-h700 --parallel 2
  '
```

The next M0 gate is a target-frontend run: start a GB/GBC game, check video/audio/input,
change a Gambatte option, save/reload SRAM and a save state, and quit/relaunch cleanly.
Then repeat under RetroArch. Automated headless Gambatte parity complements these
checks; it does not replace them. M1 starts after this gate is satisfied.

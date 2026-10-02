# libretro-battlehud

A portable battle HUD for emulated Pokémon games, implemented as a **Libretro proxy core**.

The first MVP targets Pokémon Crystal on GB/GBC with Gambatte and displays the active opponent's type(s) by reading emulated memory and compositing a small HUD into the core's software video frame.

## Design goals

- Frontend-independent: NextUI/MinArch first, RetroArch as a portability check
- No NextUI or RetroArch fork
- No process-memory hacks or root privileges
- Read-only game-state access
- Game/revision-specific memory data isolated in profiles
- Architecture ready for later DV/IV/EV support

## Initial milestones

- **M0:** transparent Libretro proxy around Gambatte
- **M1:** intercept software video frames without breaking emulation
- **M2:** expose a normalized read-only emulated-memory interface
- **M3:** decode Pokémon Crystal battle state and opponent types
- **M4:** render the type HUD
- **M5:** package and validate on NextUI
- **M6:** validate the same wrapper under RetroArch

Development starts on a feature branch and will keep the wrapper transparent before adding Pokémon-specific behavior.

## Current status and development

PR #1 implements the M0 transparent proxy, M1 opt-in video test marker and
M2 read-only memory adapter, M3 Crystal USA/Europe Rev. 1 battle decoder and
M4 pixel type icons and FIGHT effectiveness hints. Valid active battles show one or two badges at the upper right, in the
normal battle main menu and FIGHT move selection; bag/party submenus hide them.
Only SHA-1 `f2f52230b536214ef7c9924f483392993e226cfb` selects the Crystal profile.
Enable `LIBRETRO_BATTLEHUD_DEBUG=1` to see profile selection and battle changes.
See [profiles](docs/profiles.md) and [address provenance](docs/reverse-engineering.md).
M3 interactive Mac acceptance and M4 text-badge readability/main-menu visibility passed.
The new pixel icons and move hints await Mac visual acceptance.
See [move-effectiveness rules and symbols](docs/move-effectiveness.md).
Set `LIBRETRO_BATTLEHUD_DISABLE_HUD=1` to disable badges for comparison.
M0–M3 have passed desktop checks, including user-reported interactive Mac tests; see [validation status](docs/validation.md).

Set `LIBRETRO_BATTLEHUD_TEST_MARKER=1` when launching the frontend for an 8×8 white
rectangle at the top right. Without it, video remains transparent.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
export LIBRETRO_BATTLEHUD_BACKEND=/absolute/path/to/gambatte_libretro.so
```

Load `build/pokemon_gambatte_libretro.so` as the core in a Libretro frontend.
Keep test saves separate from your normal saves until device validation is complete.

See [proxy contract and validation](docs/libretro-proxy.md) and
[NextUI integration findings](docs/nextui-analysis.md).
For an Apple Silicon desktop test, see [macOS setup](docs/macos-test.md).


The isolated [NextUI H700 PBH test package](docs/nextui-package.md) is prepared for
M5; physical-device acceptance remains pending. CI publishes the package ZIP and
manifest without ROMs or a redistributed backend. It uses the firmware's original
Gambatte and keeps the ordinary GBC entry unchanged.

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
M2 read-only memory adapter for bounded Gambatte CPU mappings and raw regions.
Pokémon decoding and the type HUD are not implemented yet.
M0 and M1 have passed user-reported interactive Mac tests; see [validation status](docs/validation.md).

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

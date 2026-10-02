# Architecture

## Goal

libretro-battlehud is a frontend-independent Libretro proxy core.

The frontend loads the BattleHUD wrapper as if it were an ordinary core. The wrapper dynamically loads the real emulator core, forwards the Libretro API, observes emulated memory, and later composites a HUD into software video frames.

```text
NextUI/MinArch ─┐
RetroArch ──────┼─> BattleHUD proxy ─> real Gambatte ─> Pokémon ROM
other frontend ─┘          │
                            ├─ memory reader
                            ├─ game profile
                            ├─ battle decoder
                            └─ HUD renderer
```

## Boundaries

The portable runtime must not depend on NextUI paths, RetroArch network commands, process memory scanning, root access, or ROM patching.

Frontend-specific packaging belongs outside the core runtime.

## Milestones

- M0: transparent Gambatte proxy
- M1: video interception
- M2: normalized read-only emulated-memory access
- M3: Pokémon Crystal battle/type decoder
- M4: type HUD
- M5: NextUI package and device validation
- M6: RetroArch portability validation

DV/IV/EV and other battle data are explicitly post-MVP.

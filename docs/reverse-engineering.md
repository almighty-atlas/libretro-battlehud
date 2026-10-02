# Crystal Rev. 1 decoder provenance

## ROM and core

Only Crystal USA/Europe v1.1 / Rev. 1 is selected:
`f2f52230b536214ef7c9924f483392993e226cfb`.
The upstream [ROM hash manifest](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/roms.sha1)
identifies this as `pokecrystal11.gbc`. No ROM is bundled or downloaded.

Addresses below come from the upstream generated
[pokecrystal11.sym](https://github.com/pret/pokecrystal/blob/87b0d7436e43c3717cfc416d38a99162191bb714/pokecrystal11.sym).
The symbol blob is `90c420575df51c78db8d659474a3cd2caf13b2eb`.
Meaning and layouts were cross-checked against
[WRAM declarations](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/ram/wram.asm),
[battle structure](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/macros/ram.asm)
and [battle engine](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/engine/battle/core.asm).

| Symbol | Bank:address | Interpretation |
|---|---|---|
| `wBattleMode` | `01:D22D` | 0 overworld, 1 wild, 2 trainer |
| `wBattleEnded` | `00:C734` | Nonzero means battle ended |
| `wBattleHasJustStarted` | `01:D264` | Suppress model during initial setup |
| `wEnemyIsSwitching` | `00:C711` | Suppress model during enemy switch |
| `wEnemyMonSpecies` | `01:D206` | Species ID 1–251 |
| `wEnemyMonLevel` | `01:D213` | Validation only, 1–100 |
| `wEnemyMonHP` | `01:D216` | Big-endian current HP; zero hides fainted enemy |
| `wEnemyMonMaxHP` | `01:D218` | Big-endian validation bound, 1–999 |
| `wEnemyMonType1` | `01:D224` | First current battle type |
| `wEnemyMonType2` | `01:D225` | Second current battle type |

The engine copies the two base types into the enemy battle structure when
loading a wild or trainer opponent. The decoder reads that structure each frame,
without a species/type cache. Species is the current battle-structure value,
including changes caused by battle effects; this is not a Pokédex lookup.

## Type values

From [type_constants.asm](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/constants/type_constants.asm):

| Raw codes | Types in matching order |
|---|---|
| 0–5 | Normal, Fighting, Flying, Poison, Ground, Rock |
| 7–9 | Bug, Ghost, Steel |
| 20–27 | Fire, Water, Grass, Electric, Psychic, Ice, Dragon, Dark |

Raw 6 (Bird), 10–19 (unused/Curse), 28–255 are rejected. Gen 2 has no Fairy type.

## Gambatte address mapping

The pinned integration core is
`libretro/gambatte-libretro@d9d6cd06382d1ced30de34d56d3609452323dab1`.
Its [memory-map announcement](https://github.com/libretro/gambatte-libretro/blob/d9d6cd06382d1ced30de34d56d3609452323dab1/libgambatte/libretro/libretro.cpp)
maps `rambank0_ptr()` at C000 and `rambank1_ptr()` at D000.
Its [memory accessors](https://github.com/libretro/gambatte-libretro/blob/d9d6cd06382d1ced30de34d56d3609452323dab1/libgambatte/src/gambatte-memory.h)
return physical WRAM banks 0 and 1 (`cart_.wramdata(0)` plus 0 or 0x1000),
independently of the current CPU SVBK register. Thus `01:Dxxx` symbols are read
from physical bank 1 rather than whichever bank the CPU temporarily selects.

The decoder first uses captured Libretro mappings. If those reads are unavailable,
an explicit Gambatte-only fallback reads C000–DFFF from SYSTEM_RAM offsets
0–1FFF, requiring at least 32 KiB (a CGB WRAM allocation). This is confined to
this adapter; generic raw-region reads make no CPU-address assumption.
A different core family needs a separately verified adapter before profile use.

## Verification and limitations

Decoder fixtures cover wild Rattata (Normal), trainer Geodude (Rock/Ground), a
switch to Zubat (Poison/Flying), Gastly (Ghost/Poison), all valid Gen 2 raw types,
battle end, start/switch/faint transitions, missing memory and invalid values.
The real-Gambatte CI test runs an original GBC test ROM that writes literal
Crystal-shaped Pidgey bytes (species 16, Normal/Flying) into the stated addresses.
A host then runs the pure decoder over actual wrapper memory reads and compares
the result with the independently specified fixture. Production ROM detection
must reject that original ROM. No profile override is compiled into the product.

M3 interactive desktop acceptance was subsequently reported by the user on
2026-10-02: supported SHA-1, wild single/dual types, trainer, trainer opponent
change, battle end and save-state restoration. See [captured evidence and its
limits](validation.md). The startup backend/frontend version strings were not
captured. M4 visual HUD acceptance and physical H700 behavior remain pending.
Link/mobile and special scripted battles are not acceptance-tested.


## Main battle menu visibility (M4 refinement)

The HUD is visible only at the normal four-choice FIGHT / PKMN / PACK / RUN menu.
The combatant remains decoded while a submenu is open; visibility is a separate
`battle_state.main_menu` field. All addresses/signatures are profile data.

| Symbol / signature | Address / value |
|---|---|
| `wMenuDataPointer` | `00:CF86`, little-endian pointer must equal `4F34` |
| `wMenuDataBank` | `00:CF8A`, must equal `09` |
| `BattleMenuHeader.MenuData` | ROM `09:4F34` |
| `wTilemap` | `00:C4A0`, 20 columns |
| FIGHT at (10,14) | `C5C2`: `85 88 86 87 93` |
| PKMN at (16,14) | `C5C8`: `E1 E2` (rendered PK/MN glyphs) |
| PACK at (10,16) | `C5EA`: `8F 80 82 8A` |
| RUN at (16,16) | `C5F0`: `91 94 8D` |

Addresses come from the same pinned `pokecrystal11.sym` used by the battle profile.
The exact menu header/text comes from
[engine/battle/menu.asm](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/engine/battle/menu.asm).
[home/menu.asm](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/home/menu.asm)
copies the header, stores its ROM bank, and computes text start coordinates.
[engine/menus/menu.asm](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/engine/menus/menu.asm)
places columns six tiles apart and rows two tiles apart, and restores the previous
header on menu exit. The
[charmap](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/constants/charmap.asm)
and [text expansion](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/home/text.asm)
show that the PKMN control byte 4A renders as E1/E2.

Pointer plus bank distinguishes the normal battle menu from bag/party/move menus;
all four tile signatures additionally reject stale metadata during drawing,
restoration and action text. Missing visibility reads fail closed. Submenu entry
and return update the HUD even on NULL video duplicates. This exact signature does
not enable contest/mobile/other special menu layouts; those remain out of scope.
CPU tilemap updates and displayed frames can have transitional timing differences,
so the actual bag/party/move entry/return behavior still requires the Mac test.

# Party DVs and stat experience on Crystal's third stats tab

The blue (third) stats page shows a lower-left DV/EV table for the selected
player-party Pokémon. It replaces the trainer ID/OT pane at x=0..79, y=64..143;
the original stat values, vertical divider, portrait, nickname and page controls
remain intact. Rows are HP, ATK, DEF, SPA, SPD, SPE. SPA/SPD mean Special Attack
and Special Defense; SPE means Speed.

DVs are 0..15. HP DV is constructed from the least-significant bits of Attack,
Defense, Speed and Special (bits 3, 2, 1, 0 respectively). Both Special stats
share one DV. EV here means Gen 2's raw stat experience, 0..65535, not modern
0..252 EVs or a calculated stat bonus. Five big-endian counters are stored in
the Pokémon structure; both Special rows show the same Special counter.
The footer explicitly labels these values STAT EXP.

## Read-only page and identity checks

Detection requires blue-page flags, stable MonStatsJoypad state 6, player-party
source 0, a valid party slot/count, valid species/level, the blue-page indicator
and all five rendered stat labels. The displayed 48-byte temporary Pokémon must
exactly match the currently selected party structure. This hides the overlay
during page/party transitions and prevents displaying the previous Pokémon's
values. Eggs, boxes, opponent-party screens and unrelated menus are excluded.
No ROM/RAM edits and no Save RAM writes are performed.

The ordinary HUD-disable switch disables this table as well. It works outside
battle and from the player's party stats during battle. The existing clean-frame
cache replaces changed training data on NULL duplicate frames and removes the
pane when switching tabs/exiting; reset/restore/load/unload still clear the cache.
The overlay requires the exact 160x144 software frame layout.

## Pinned sources and addresses

Source revision: `pret/pokecrystal` at
`5beda23ffa505f62e1dad7e3d7c214d1737b3358`:

- [Stats screen, page state and placement](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/engine/pokemon/stats_screen.asm)
- [Party-to-temporary copy](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/engine/pokemon/tempmon.asm)
- [Original stat labels](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/engine/pokemon/mon_stats.asm)
- [Pokémon structure definitions](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/constants/pokemon_data_constants.asm)
- [Character tiles](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/constants/charmap.asm)

Rev. 1 `pokecrystal11.sym`, symbols revision
`87b0d7436e43c3717cfc416d38a99162191bb714`:

| Field | CPU address |
| --- | --- |
| wStatsScreenFlags / wJumptableIndex / wMonType | CF64 / CF63 / CF5F |
| wCurPartyMon / wPartyCount / wCurPartySpecies | D109 / DCD7 / D108 |
| wPartyMon1 / stride | DCDF / 48 bytes |
| wTempMon / wTempMonStatExp / wTempMonDVs | D10E / D119 / D123 |
| Blue-page top indicator tiles | C515 / C516 = 3A / 3B |
| ATTACK / DEFENSE / SPCL.ATK / SPCL.DEF / SPEED text | C54B / C573 / C59B / C5C3 / C5EB |

The Dxxx addresses use the existing Gambatte physical WRAM bank-1 adapter.

## Validation

Tests exercise all six party slots, all sixteen HP-DV parity combinations,
shared Special data, endian conversion, zero/65535 stat experience, invalid
party data, mismatched temporary data, missing reads, stale labels, eggs and
other pages/sources. Pixel tests cover all three formats, the exact pane bounds,
header/value pixels, immutable input, duplicate updates and pane removal.
Training/decoder/renderer tests pass ASan+UBSan. The synthetic renderer preview
was visually inspected. Interactive Crystal Mac acceptance of this new table
and physical H700 acceptance remain pending. The user accepted the preceding
pixel type icons and FIGHT move hints before this change.

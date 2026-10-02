# Party training values for Gen 1, 2 and 3

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
The EXP header explicitly labels these values as stat experience; the footer
now shows [Hidden Power](hidden-power.md) type and base power.

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
was visually inspected. Interactive Crystal Mac acceptance passed: the user verified the table changes
with each selected Pokémon. Physical H700 acceptance remains pending. The user accepted the preceding
pixel type icons and FIGHT move hints before this change.


## English Red and Blue (Gen 1)

Supported original SHA-1s: Red `ea9bcae617fdf159b045185467ae58b2e4a48b9a`,
Blue `d7037c83e1ae5b39bde3c30787637ba1d4c48ce2`. Both require Gambatte.
The first stats page has five rows: HP, ATK, DEF, SPE, SPC. Special is a single
stat in Gen 1; values remain DVs 0–15 and raw stat experience 0–65535.
The table replaces the lower-right types/OT/ID pane at x=80..159, y=72..143,
leaving all original stat numbers, name, portrait, HP and level intact.
Source must be the player's party, all four stat labels and TYPE signature must
be present, and the displayed 44-byte structure must equal the selected party
entry. The moves/EXP page, level-up popups, boxes and opponents do not qualify.
Gambatte's non-CGB 8 KiB SYSTEM_RAM layout is explicitly supported in the fallback.

Pinned source: [pret/pokered](https://github.com/pret/pokered/blob/d2704a63c26f9ba046ade877445216b3de0519a4/engine/pokemon/status_screen.asm),
[symbols](https://github.com/pret/pokered/blob/3f618d59edf43918f48f5e558c34e04cb2fc5619/pokered.sym).
Red/Blue WRAM: party count D163, base D16B, stride 44; selected slot CF92,
source CC49; loaded mon CF98; DVs CFB3, big-endian stat experience CFA9..CFB2.
Stats tile signatures C455/C47D/C4A5/C4CD, TYPE at C45E; tilemap C3A0.

## English Emerald (Gen 3)

Supported original SHA-1: `f3ae088181bf583e55daf962a92bb46f4f1d07b7`, backend **mGBA**.
The profile also has a [separate battle decoder](multigen-battles.md). Crystal
battle addresses are never applied to Gen 1/3. No other GBA core or edition is enabled.

The Pokémon Skills page (second tab) gets two three-row columns for HP/ATK/DEF
and SPA/SPD/SPE. IVs are 0–31; stored EVs are 0–255, with a 510 total limit.
The footer shows total EVs out of 510. A 255 counter is shown faithfully rather
than capped at 252. The overlay replaces x=80..239, y=112..159 (EXP/next-level
pane), preserving the portrait, nickname, level, held item and original stats.
It requires exact 240x160 software video, an active settled summary input task,
summary callback, no palette fade, Skills page, party list and valid slot/count.
The displayed 100-byte mon must exactly match the corresponding player-party mon.
Eggs, Bad Eggs, boxes, other pages, transitions and corrupt data are hidden.

The 48 encrypted bytes are copied locally and XOR-decoded with PID XOR OT ID.
PID modulo 24 determines the four block positions; the 16-bit checksum must
match. Six byte EVs and six 5-bit IVs are reordered to the displayed stat order.
No game RAM, save data or ROM is changed by the decoder.

Pinned source [pret/pokeemerald](https://github.com/pret/pokeemerald/blob/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881/src/pokemon_summary_screen.c),
[Pokémon layout](https://github.com/pret/pokeemerald/blob/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881/include/pokemon.h),
[encryption and permutation](https://github.com/pret/pokeemerald/blob/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881/src/pokemon.c),
[symbols](https://github.com/pret/pokeemerald/blob/dba968c67d85caf9595abe12a51ff739d4dc5937/pokeemerald.sym).
Party count 020244E9, base 020244EC, stride 100; summary pointer 0203CF1C;
gMain.callback2 030022C4 must equal 081BFAB5; gTasks 03005E00 (16 × 40),
active input function 081C0511; gPaletteFade 02037FD4, active bit 7 of byte 7.
Summary monList +0, currentMon +0C, mode/isBox/slot/max/page +40BC..40C0.
Pointers must stay aligned and within EWRAM for every required read.

mGBA provides explicit EWRAM/IWRAM descriptors. Their mappings are verified from
[pinned Libretro adapter](https://github.com/mgba-emu/mgba/blob/c3c8e5e813f245028de118a56734e1dc0f35ce2a/src/platform/libretro/libretro.c).
There is no guessed GBA SYSTEM_RAM address fallback. Missing maps hide the overlay.

## New automated coverage and acceptance

Both Red/Blue profiles, all party slots and HP-DV parity combinations; all 24
Emerald encrypted arrangements across all six slots; shared Special, distinct
modern Special stats, checksum failures, eggs, invalid pointers, EV totals and
page/task/fade/identity guards are covered. Renderer checks all three pixel
formats, exact generation-specific bounds, unchanged source pixels, cache
updates and removal. CI builds real pinned Gambatte and mGBA on Linux and Apple
Silicon and compares direct/proxy emulation using original test programs.
Emerald-shaped mapped RAM and renderer fixtures do not replace manual testing
with Emerald. Red/Blue and Emerald interactive macOS acceptance remains pending.
The existing NextUI PBH package still launches Gambatte; it cannot launch Emerald.

The Gen 2/3 training panes now also show [Hidden Power type/base power](hidden-power.md).

Emerald also shows [nature arrows and stored ability](nature-ability.md) above the
training table. Its additional banner occupies y=104..111; original stats end at
y=103 and remain untouched.

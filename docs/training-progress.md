# Training progress (issue #4)

On supported party stats pages, `BattleHUD: Training view` selects `raw`
(default), `bonus`, or `gains`. The DV/IV column stays visible. Compact layout
omits this second column and its footer. Hidden Power and nature/ability remain
independent. `NA` means a required value or comparison is unavailable.

| View | Header | Meaning |
| --- | --- | --- |
| raw | EXP (Gen 1/2), EV (Gen 3) | Stored training values, without modern caps applied to older generations |
| bonus | BON | Calculated stat points supplied by training at this Pokémon's current level |
| gains | GAIN | Observed change in raw training values over the last complete, recognized battle |

Emerald's raw/bonus footer uses `EV510 R0`, for example: 510 EVs used and zero
remaining. Budget is 510 total; each stored byte may reach **255**, while 252–255
all supply the same EV term. Gen 1/2 have neither this total budget nor a 252/255
per-stat training cap: their raw Stat EXP ranges from 0 to 65535. Crystal shares
Special training experience and DV between SPA/SPD, but uses distinct base stats.

## Exact bonus calculation

BON is `calculated_stat(training) - calculated_stat(0)`, holding species, level,
DVs/IVs and nature fixed. It is not an estimate of the next level-up gain or a
claim that the game has already refreshed its stored stat numbers.

For Gen 1/2, the training term is
`floor(min(255, ceil(sqrt(StatEXP))) / 4)`. The original engines start the root
search at one; this also gives zero contribution for Stat EXP zero.
The stat is `floor((2*(base+DV) + term)*level/100) + offset`, capped at 999.
HP's offset is `level+10`; other stats use 5. Each stat calculation is rounded
before taking the difference. Gen 1 has a single Special stat.

For Gen 3, the term is `floor(EV/4)` and the inner sum is `2*base + IV + term`.
The same level division and offsets apply; non-HP stats then receive the stored
nature's 110%, 90%, or 100% multiplier with another integer division. Shedinja
has one HP irrespective of EVs, so its HP contribution is zero. Missing base
stats hide only the bonus values; raw training stays usable.

## Observed gains and limitations

The observer reads all party members once per `retro_run`, regardless of the
selected training view. It saves a valid outside-battle snapshot, compares it
with the party when a recognized battle starts, and accepts the final result
only after two equal outside-battle training snapshots. It never infers awards
from the opponent, EXP, Pokérus or a predicted yield. Zero means an observed
zero change; `NA` means there was no trustworthy comparison.

Identity includes species, OT ID, DVs and both OT/nickname strings in Gen 1/2;
Gen 3 uses PID, OT ID, nickname, IV/ability bits and species. The entire ordered
roster must match. Reordering, switching party members, evolution, renaming,
catches that alter the party, eggs, ambiguous identical identities, unavailable
reads or invalid/unsupported battle states discard the comparison. Counters
must never fall below their baseline. After a result, any party/training change
outside battle discards it; level-only changes do not create training gains.
A catch sent to a full PC box leaves the roster untouched: only a real raw
counter difference can appear, including an observed zero.

Reset, unload, ROM change, successful Libretro unserialize (including rewind),
and deinit clear the observer. Starting the proxy in the middle of a battle
has no outside baseline and cannot report gains for that battle. The observer
is not serialized. Identical clones or changes entirely between sampled frames
cannot be distinguished beyond these conservative identity checks; this is an
observation of RAM changes, not an award-attribution system.

GB/GBC base stats are read from a private immutable copy of the exact recognized
ROM, using a virtual read-only address range. The copy is checksum-verified again
and freed on unload/deinit/new content. Gen 3 uses mapped ROM descriptors.
No game ROM, RAM, SRAM or backend savestate is modified.

## Provenance

- [Red/Blue CalcStats/CalcStat](https://github.com/pret/pokered/blob/d2704a63c26f9ba046ade877445216b3de0519a4/home/move_mon.asm)
- [Crystal CalcMonStats/CalcMonStatC](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/engine/pokemon/move_mon.asm)
  and [GetSquareRoot](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/engine/math/get_square_root.asm)
- [Emerald CalculateMonStats/CALC_STAT/ModifyStatByNature](https://github.com/pret/pokeemerald/blob/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881/src/pokemon.c)
- Existing pinned symbol sets in [reverse engineering](reverse-engineering.md):
  Red/Blue BaseStats `0e:43de`, MewBaseStats `01:425b`; Crystal 1.1 BaseData
  `14:5424`. Base records have strides 28 and 32, respectively. Gen 1's
  internal species IDs use the existing checked National Dex mapping.

## Validation

Local automated checks cover every 16-bit Stat EXP value in both older
generations, zero/boundary/rounding cases at levels 1, 5, 50 and 100, 255 stored
EVs, 510 total, positive/negative nature rounding and Shedinja. Actual Gen 1/2/3
party-decoder fixtures verify base-stat ordering, shared Special, identities,
all encrypted Gen 3 permutations, checksum failures and duplicate rejection.
Observer fixtures exercise two Pokémon and all three generations through
multiple battles, reorder/catch/deposit/replacement/clone cases, reset/load
clearing, missing data, decreases, unstable endings and mid-battle startup.
Raw/BON/GAIN/NA switching and removal use clean duplicate frames in all three
software pixel formats; renderer fixtures were visually inspected. Address
and undefined-behavior sanitizer checks pass for the observer/formulas.

Interactive acceptance of these new views on macOS and NextUI remains pending.
Existing accepted Crystal badges/FIGHT/raw training checks remain accepted and
need not be repeated. A new focused check should switch the three views on one
stats page and, for GAIN, complete a new battle after launching this build.

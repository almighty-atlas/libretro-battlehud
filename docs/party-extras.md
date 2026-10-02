# Optional party details

Select **BattleHUD: Training view → details** with **Training values** enabled.
The validated party stats page shows details instead of the DV/IV and training
table. Raw remains the default. Compact does not reduce the detail panel.
Hidden Power and the Emerald nature/ability banner keep their independent toggles.

| Detail | Crystal Rev. 1 | Emerald |
|---|---|---|
| FRIEND | Stored friendship 0–255 | Stored friendship 0–255 |
| SEX | Attack/Speed DVs and species ratio | PID low byte and species ratio |
| SHINY | Defense/Speed/Special DVs 10 and Attack DV bit 1 | XOR of PID and OT ID halves below 8 |
| PKRS | NONE / ACTIVE / CURED | NONE / ACTIVE / CURED |
| Last line | PKRS DAYS: stored low nibble | PKRS EV X1/X2: Pokérus factor for battle EV gain |

SEX NONE means genderless; SEX NA means unavailable ROM data. Male-only and
female-only species have explicit branches. Gen 2 compares the combined
Attack/Speed byte **≤** ratio; Gen 3 compares PID low byte **<** ratio.
Shiny and gender are calculated; friendship and Pokérus are stored.
ACTIVE means contagious (nonzero low nibble). CURED means a nonzero byte with
zero low nibble. In Gen 3, cured Pokémon retain X2 because MonGainEVs checks
the whole byte. This shows only the Pokérus factor: held-item multipliers,
participation and EV caps are not included. Ordinary EXP is not doubled.
Gen 2 DAYS is the raw countdown, not a prediction or total training multiplier.

Red/Blue retain the raw training table when details is selected. No Gen 1
friendship, gender, Shiny or transfer properties are inferred. Eggs and boxes
remain excluded. All existing page/transition/checksum/selected-party identity
guards apply. Missing gender lookup shows NA while valid other details remain
available. Property changes redraw duplicate frames. Decoding never writes RAM,
ROM or save data.

Crystal replaces x=0..79,y=64..131 with five rows. Emerald replaces
x=80..239,y=112..147 with two columns. Original stats, portrait, level, Emerald
held item and nature/ability banner remain visible. Details replace the EV total
as well as the training table; the optional Hidden Power footer stays visible.

## Pinned primary sources

Crystal commit `5beda23ffa505f62e1dad7e3d7c214d1737b3358`:
- [Structure offsets](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/constants/pokemon_data_constants.asm): friendship +27, Pokérus +28, DVs +21/+22.
- [GetGender](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/engine/pokemon/mon_stats.asm): ratio at BaseData +13, inclusive comparison.
- [CheckShininess](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/engine/gfx/color.asm).
- [Contagious check](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/engine/events/pokerus/check_pokerus.asm).

Emerald commit `731ad5bfd6e6f265508d0efcca0ba42f9dcf5881`:
- [Structures](https://github.com/pret/pokeemerald/blob/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881/include/pokemon.h): Growth friendship +9, Misc Pokérus +0, SpeciesInfo ratio +16.
- [Routines](https://github.com/pret/pokeemerald/blob/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881/src/pokemon.c): GetGenderFromSpeciesAndPersonality, IsShinyOtIdPersonality, CheckPartyPokerus, CheckPartyHasHadPokerus, MonGainEVs.
ROM table addresses use the existing exact [training profiles](training-stats.md).

## Verification

Automated fixtures cover every Gen 2 DV pattern, Gen 3 Shiny threshold, all
256 ratios × 256 determinants in each generation, every Pokérus byte, fixed
sex/genderless cases, missing reads, Gen 1 exclusion, friendship limits, six
party slots and all 24 encrypted Gen 3 block arrangements. Rendering checks all
three pixel formats, panel bounds, clean input, duplicate property changes,
disable and cleanup.

Manual macOS/NextUI acceptance is pending. Select details on the accepted stats
page, switch Pokémon, restore raw, and leave the summary. Original stat numbers
must remain visible and the panel must disappear on exit. Automated synthetic
Shiny/Pokérus cases are not manual acceptance with a real save.

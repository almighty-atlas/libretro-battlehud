# Emerald nature bonuses and stored ability

English Emerald's settled player-party Skills page now shows a small banner
above the IV/EV table: `N <nature>` on the left and the ability name on the right.
The banner occupies x=80..239, y=104..111, the gap after the original stat
windows ending at y=103 and before the original EXP area beginning at y=112.
The existing training table, EV total and Hidden Power row remain visible.
Portrait, nickname, level, held item and all original stat numbers stay intact.

A green 5x7 up arrow beside the appropriate training-row label marks the stat
raised by the nature; a red down arrow marks the lowered stat. The arrows refer
to the Pokémon's **stat**, not to its IV/EV counter: those raw numbers stay
unchanged. Neutral natures have no arrows; HP is never modified by a nature.
These markers express 110%/90% nature modifiers, not an exact flat stat increase.
Gen 1/2 do not show these features.

## Read-only decoding and provenance

The existing summary callback/task/page/fade, selected slot/count, checksum and
100-byte displayed-to-party identity checks must pass before details are decoded.
Nature is `personality % 25`; the ordered 25-nature table raises the quotient
`nature / 5` stat and lowers the remainder `nature % 5` stat, except equal indices
are neutral. Nature table order is Attack/Defense/Speed/Special Attack/Special
Defense and is explicitly converted to displayed HP/ATK/DEF/SPA/SPD/SPE order.

The ability selection comes from bit 31 of the validated IV/Misc word, **not**
PID parity. The actual ID is read from the selected species' ability slot in
Emerald's ROM. Both slots are respected. An absent second ability is displayed
as NONE if that slot was actually stored; no fallback to the first is invented.
This is the stored party ability, not a battle-only replacement such as Trace.

Pinned [pret/pokeemerald pokemon.c](https://github.com/pret/pokeemerald/blob/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881/src/pokemon.c)
provides GetNature, gNatureStatTable, MON_DATA_ABILITY_NUM and GetAbilityBySpecies.
[Pokémon layout](https://github.com/pret/pokeemerald/blob/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881/include/pokemon.h)
contains the bitfield and SpeciesInfo layout;
[species records](https://github.com/pret/pokeemerald/blob/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881/src/data/pokemon/species_info.h)
and [ability names](https://github.com/pret/pokeemerald/blob/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881/src/data/text/abilities.h)
provide the lookup provenance.

Symbols revision [dba968c67d85caf9595abe12a51ff739d4dc5937](https://github.com/pret/pokeemerald/blob/dba968c67d85caf9595abe12a51ff739d4dc5937/pokeemerald.sym):

| Field | Address / layout |
| --- | --- |
| gSpeciesInfo | 083203CC; 412 entries, stride 28 |
| Abilities | SpeciesInfo +22 / +23 |
| gAbilityNames | 0831B6DB; 78 entries, stride 13 including EOS |

IDs must be 0..77. Names are read through mGBA's existing ROM descriptors and
converted using the pinned [English charmap](https://github.com/pret/pokeemerald/blob/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881/charmap.txt).
Up to 12 letters/spaces are accepted with an EOS within the 13-byte entry;
lowercase letters normalize to uppercase for the HUD font. No name is truncated.
ID 0 is represented semantically as NONE rather than the game's seven dashes.
Missing ROM reads, invalid IDs or malformed names hide the ability alone; valid
training data, nature and Hidden Power remain available. No game or save writes.

## Cache behavior and validation

Nature, ability ID/slot/name and their validity flags are part of training-state
equality. A change redraws NULL duplicate frames; a shorter name erases the old
tail, loss of details restores the original banner area, and leaving the Skills
page removes the full overlay. Reset/load/unload/restore cleanup remains in place.
The global HUD-disable switch disables these details too.

Tests cover all 25 literal nature-table rows, five neutral natures, HP neutrality,
both stored ability slots, an odd PID with slot 0, zero second ability, all valid
IDs, 12-character names, lowercase input, missing reads and invalid text/control
codes. Decoders test nature and ability after all 24 encrypted arrangements and
across the six slots. Pixel tests cover all three formats, arrows/direction and
banner bounds, unchanged original pixels, name-only changes, removal and malformed
models. The real-mGBA original-program fixture includes small factual ROM lookup
entries and verifies Hardy/Overgrow from mapped data; it remains rejected by the
production ROM gate. It does not include a commercial ROM or BIOS.

A synthetic Adamant/Jolly/Hardy preview was visually reviewed. Automated tests
and sanitizers complement that preview; interactive Emerald acceptance remains
pending. Track implementation and acceptance in [issue #3](https://github.com/almighty-atlas/libretro-battlehud/issues/3).

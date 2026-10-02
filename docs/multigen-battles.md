# Red/Blue and Emerald battle HUD

English Red/Blue with Gambatte and English Emerald with mGBA now have separate
battle profiles, in addition to their party training tables. The exact hashes in
[profiles](profiles.md) remain mandatory. Crystal uses its existing decoder and
Gen 2 rules; none of its RAM addresses or menu signatures are reused for Gen 1/3.
The existing [HUD options](hud-options.md) apply to all three generations.

## Menus and rendering

Ordinary wild/trainer battles show current opponent type icons in the settled
main menu and FIGHT. Bag, party-selection menus, action text, fades, initial
send-out, fainting, switching and invalid/missing data hide battle overlays.
Party stats pages retain their independently gated training display.

Red/Blue's four move hints use x=144..150, y=104..110 / 112..118 / 120..126 /
128..134 in the 160x144 list, preserving names, cursor and PP. Emerald's 240x160
move grid uses x=1..7 and x=153..159, y=124..130 and y=140..146. The first column
marker uses the decorative left border; the second uses the gap after the move
name window. Text windows x=16..79 / 88..151, cursors x=8..15 / 80..87 and the
PP/type pane x>=168 are preserved. Move IDs are 16-bit, including moves 256–354.

In an ordinary Emerald double battle, all live/non-absent opponent positions
are counted. If two foes remain, no unique target has yet been selected: badges
are hidden and FIGHT hints show `?` (or the no-PP/Disable square). They do not
claim that the last RAM opponent is the selected target. When only one opponent
remains, its current types/ability are used. Target-selection screens themselves
are outside the main/FIGHT controller gate. Link, Safari, tutorial, Frontier,
recorded and other alternate battle-controller modes are excluded.

The software compositor preserves core-owned pixels and rebuilds changed NULL
duplicate frames from a clean copy. Stale icons/hints therefore disappear on
menu changes, target ambiguity, foe changes and battle end.

## Generation-specific rules

The [same symbols](move-effectiveness.md) describe combined **type** effectiveness,
plus directly known ability immunities. They do not predict exact damage,
accuracy, STAB, Protect/Substitute, sleep/charge timing, weather damage bonuses,
Thick Fat, screens, survival, or whether an ordinary move will succeed.

Gen 1 uses its original 15-type table: Ghost attacks fail against Psychic;
Poison and Bug are super effective against each other; Ice is neutral against
Fire. Karate Chop, Gust and Bite are Normal. Struggle is still Normal and cannot
hit Ghost. The SetDamageEffects path bypasses type calculation for special/fixed
damage and Super Fang: Seismic Toss can hit Ghost and Night Shade can hit Normal.
The hint reflects combined damage factors; Gen 1's last-matching-pair battle
message bug does not change the combined factor. Counter/Bide/copying moves are
unknown. OHKO and Dream Eater remain conditional unless a chart immunity is known.

Gen 3 uses its original 17-type table, including Steel's Ghost/Dark resistances.
Both current types and the **actual battle ability** are read, so Transform,
Conversion, Trace and Skill Swap are represented by live data. Foresight removes
Ghost's Normal/Fighting immunity. Hidden Power derives its type from battle IVs.
Struggle bypasses chart and Wonder Guard. Fixed damage ignores type resistances
but retains chart and ability immunities. OHKO/Counter/Mirror Coat/Endeavor,
Dream Eater, Snore and Spit Up are conditional rather than promised outcomes.

Known ability blocks: Levitate (Ground), Volt Absorb (powered Electric), Water
Absorb (powered Water), Flash Fire (Fire unless the defender is frozen), Soundproof
(the game's exact sound-move list), Wonder Guard (non-super-effective powered
moves) and Sturdy (OHKO). Non-damaging moves use the status dash, except a known
Soundproof/Flash Fire block. Thunder Wave is not classified as an ordinary attack
and is not incorrectly blocked by Gen 3 Volt Absorb. Missing context/invalid
ability IDs never become a confident neutral hint. Weather Ball, Nature Power,
Metronome, Mirror Move, Assist, Sleep Talk, Bide, Beat Up, Future Sight and Doom
Desire remain unknown: their dynamic, party, delayed or target-dependent rules
are deliberately not guessed from the nominal move type.

## Address and source provenance

Red/Blue symbol map: [pret/pokered 3f618d5](https://github.com/pret/pokered/blob/3f618d59edf43918f48f5e558c34e04cb2fc5619/pokered.sym).
Source facts pinned at [d2704a6](https://github.com/pret/pokered/tree/d2704a63c26f9ba046ade877445216b3de0519a4):
[core](https://github.com/pret/pokered/blob/d2704a63c26f9ba046ade877445216b3de0519a4/engine/battle/core.asm),
[moves](https://github.com/pret/pokered/blob/d2704a63c26f9ba046ade877445216b3de0519a4/data/moves/moves.asm),
[chart](https://github.com/pret/pokered/blob/d2704a63c26f9ba046ade877445216b3de0519a4/data/types/type_matchups.asm),
[fixed path](https://github.com/pret/pokered/blob/d2704a63c26f9ba046ade877445216b3de0519a4/data/battle/set_damage_effects.asm),
[menu text](https://github.com/pret/pokered/blob/d2704a63c26f9ba046ade877445216b3de0519a4/data/text_boxes.asm),
[species order](https://github.com/pret/pokered/blob/d2704a63c26f9ba046ade877445216b3de0519a4/data/pokemon/dex_order.asm).
Internal species IDs are mapped to National Dex for battle diagnostics; all
MissingNo. entries are rejected. Menu checks require both geometry and rendered
tile signatures, not a stale text-box ID alone.

| Red/Blue datum | Address / check |
| --- | --- |
| Mode / battle kind / escaped / initial send-out | D057 / D05A / D078 / D11D |
| Enemy species / identity / level / HP / max HP / types | CFE5 / CFD8 / CFF3 / CFE6 / CFF4 / CFEA..CFEB |
| Main menu | text-box D125=0B, CC24=14, CC25=9 or 15, CC28=1, FIGHT/PKMN/ITEM/RUN tile signatures |
| FIGHT | CCDB=0, CC24=12, CC25=5, CD6C=rows−1, CC28=rows+1, four box-corner signatures |
| Player moves / PP / Disable | D01C / D02D / D06D |

Emerald symbols: [dba968c](https://github.com/pret/pokeemerald/blob/dba968c67d85caf9595abe12a51ff739d4dc5937/pokeemerald.sym).
Facts/layout at [731ad5b](https://github.com/pret/pokeemerald/tree/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881):
[player controller](https://github.com/pret/pokeemerald/blob/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881/src/battle_controller_player.c),
[battle main/chart](https://github.com/pret/pokeemerald/blob/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881/src/battle_main.c),
[move facts](https://github.com/pret/pokeemerald/blob/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881/src/data/battle_moves.h),
[type scripts](https://github.com/pret/pokeemerald/blob/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881/src/battle_script_commands.c),
[ability blocks](https://github.com/pret/pokeemerald/blob/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881/src/battle_util.c),
[BattlePokemon](https://github.com/pret/pokeemerald/blob/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881/include/pokemon.h),
[windows](https://github.com/pret/pokeemerald/blob/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881/src/battle_bg.c).
RAM species IDs remain Emerald internal IDs (e.g. Treecko=277), as in its training
profile. Lookup facts are compiled numeric data; no ROM assets are bundled.

| Emerald datum | Address / check |
| --- | --- |
| Main callback / mode / outcome / fade | 030022C4=08038421 / 02022FEC / 0202433A=0 / 02037FDB bit 7 clear |
| Count / position IDs / absent mask | 0202406C / 02024076 / 02024210; count=2 or 4, distinct positions |
| Combatants | 02024084, four × 88 bytes; current HP, level, species/types/ability validated |
| Controllers / execution mask | 03005D60 / 02024068; exactly one waiting eligible player controller |
| Main / FIGHT waiting functions | 08057589 / 08057BFD (Thumb bit required) |
| Command buffers | 02023064, four × 512; command 18/action or 20/moves |
| BG0 scroll | 02022E14 / 02022E16; X=0, Y=160/action or 320/moves |
| Disable records | 020242BC, four × 28; move +4, timer low nibble +11 |

The controller command, waiting function, execution bit and scroll must agree.
FIGHT's ChooseMoveStruct must agree with the live player species, types, moves
and current PP; missing or inconsistent snapshots suppress move hints. All game
reads use the existing explicit Gambatte/mGBA mappings; no memory writes occur.

## Verification and acceptance

Automated fixtures cover both Kanto profiles, wild/trainer modes, opponent
changes, start/end/fainting, Safari exclusions, stale geometry/tiles, PP/Disable,
MissingNo., type errors, callback/Thumb/command/fade/scroll gates, mapped positions,
actual ability changes, double-target ambiguity and move IDs above 255. Exhaustive
single/dual chart comparisons retain Gen 1's four differences and compare Gen 3
against the existing independently derived Gen 2 table. Pixel tests cover all
software formats, exact hint bounds, preserved names/cursors/PP, NULL erasure and
immutable inputs. Pinned real Gambatte/mGBA integrations use original programs
and paused RAM fixtures; synthetic ROMs stay unrecognized in production.

Interactive Red/Blue/Emerald macOS acceptance and physical H700 acceptance remain
pending. Automated fixtures verify the decoder/compositor, not a full interactive
playthrough of commercial ROMs. The NextUI PBH launcher still uses Gambatte;
Emerald needs a separate mGBA setup.

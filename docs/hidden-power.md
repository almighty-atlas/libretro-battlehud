# Hidden Power type and base power

The selected party Pokémon's training page shows `HPWR`, a colored type icon,
and `P nn` for Hidden Power's base power. It is visible in Crystal Rev. 1 (third
blue tab) and Emerald US (second Skills tab), using the existing exact-ROM/backend
and stable-page/party identity checks. The values are shown even if the Pokémon
has not learned Hidden Power, making them useful for planning a moveset.
Gen 1 has no Hidden Power and keeps its existing five-row stats table.

Crystal's six rows move up by one pixel within the same x=0..79, y=64..143 pane.
The counter header becomes EXP to identify raw stat experience. The old footer
is replaced by HPWR, type tile at (28,132), and power text. Emerald retains its
EV total footer on the left; the HPWR row occupies the right half, with the type
tile at (188,148). Its three stat rows move up one pixel. All original pixels
outside each existing training pane remain unchanged. Full 12x12 icons use the
same hand-authored 8x8 silhouettes as the battle HUD.

`P` means **base power**, before STAB, attack/defense stats, damage randomness or
other battle modifiers. Values are derived solely from that Pokémon's DVs/IVs,
not from the opponent. The row is hidden for unsupported generations or invalid
values. It follows the ordinary HUD-disable switch and appears/disappears with
the training table. Changes on NULL duplicate frames reuse the clean frame and
redraw, including type or power changes, without stale icons.

## Formulas and sources

The canonical type order for both generations is Fighting, Flying, Poison,
Ground, Rock, Bug, Ghost, Steel, Fire, Water, Grass, Electric, Psychic, Ice,
Dragon, Dark. Normal and the unused Bird/Mystery slot never occur.

Gen 2 DVs A=Attack, D=Defense, V=Speed, S=Special, each 0..15:

- Type index: `(A & 3) * 4 + (D & 3)`.
- Let `b = (A >> 3)*8 + (D >> 3)*4 + (V >> 3)*2 + (S >> 3)`.
- Power: `floor((5*b + (S & 3))/2) + 31`, range 31..70.
- HP DV and the duplicated Special Defense row do not affect the calculation.

Source: [pret/pokecrystal HiddenPowerDamage](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/engine/battle/hidden_power.asm).
The Crystal FIGHT effectiveness hint now shares the same canonical type helper,
so its type calculation cannot diverge from the party row. Its existing move
rules, PP/Disable and type-factor logic are unchanged.

Gen 3 IVs are reordered from displayed HP/ATK/DEF/SPA/SPD/SPE to game bit order
HP/ATK/DEF/SPE/SPA/SPD. Let `t` be their low bits as a six-bit integer, and `p`
their second-lowest bits in the same order:

- Type index: `floor(15*t/63)`.
- Power: `floor(40*p/63) + 30`, range 30..70.
- Higher IV bits and all EVs do not affect either value.

Source: [pret/pokeemerald Cmd_hiddenpowercalc](https://github.com/pret/pokeemerald/blob/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881/src/battle_script_commands.c).
The decoded Emerald IVs already passed encryption/checksum/identity validation.
The helper reads local data only. It never changes ROM, RAM, moves or save data.

## Validation

Known vectors include the user's Crystal Totodile spread
HP/ATK/DEF/SPA/SPD/SPE = 6/14/3/0/0/3 -> Electric, power 51;
and the Emerald fixture 31/1/17/29/3/2 -> Ice, power 56.
Tests enumerate every 65536 Gen 2 Attack/Defense/Speed/Special combination and
all 4096 Gen 3 type/power bitmask pairs. They cover all 16 types, rounding,
minimum/maximum power, irrelevant high IV bits, input immutability, unsupported
generations and out-of-range data. Pixel tests check actual icon/power pixels,
all three formats, separate type/power duplicate updates, invalid-row removal,
existing pane bounds and full overlay removal. The real-mGBA mapped-RAM fixture
also computes Ice/56 from its decoded IVs. The synthetic renderer preview was
visually inspected. Interactive acceptance of this row remains pending.

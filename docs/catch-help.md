# Crystal catch estimate (issue #7)

The initial catch helper supports the exact English Crystal Rev. 1 + Gambatte
profile. `BattleHUD: Catch estimate` (`battlehud_catch`) defaults to enabled.
In a recognized wild-battle **BALL pocket scrolling list**, the selected ball's
estimate appears in a small panel replacing the pack illustration. The panel
is x=0..47, y=16..51 on the 160×144 software frame. Ball names, quantities,
cursor, pocket title and description remain visible. Type badges and FIGHT
hints stay hidden in the bag. An independent option hides the catch panel.

The display uses `CATCH`, e.g. `~33.5%`, and the selected ball's short name.
The tilde marks an estimate based on the original algorithm and a uniform RNG
model, not the next PRNG outcome, a predicted shake count or a promise that the
next throw succeeds. Percentages are truncated to one decimal place, so an
estimate below 100% cannot be rounded up to 100%. `NA` means incomplete or
ambiguous inputs. Debug logging reports the untruncated basis-point estimate.

## Original Crystal rules

The helper reads the actual highlighted inventory record, its nonzero quantity,
current enemy catch-rate byte, current/max HP, status and relevant ball inputs.
The first modified catch-rate byte is saturated just as in the ROM.

| Ball | Pinned Crystal behavior |
| --- | --- |
| Poké / Friend / Moon | Base rate; Moon's Burn Heal evolution-test bug means no species receives its intended boost |
| Great | Rate + floor(rate/2), saturated at 255 |
| Ultra | Twice the rate, saturated at 255 |
| Level | ×2/4/8 only when player level, floor(level/2), floor(level/4) is strictly greater than enemy level; this ball skips HP and status calculations |
| Lure | ×3 only for fishing battles |
| Fast | ×4 only for Magnemite, Grimer and Tangela, preserving the three-entry lookup bug |
| Heavy | Reads the ROM's dex pointers/bank table and weight bytes; uses the original pounds conversion and ±20/+30/+40 branches, including the wrong-bank behavior for species 64/128/192 |
| Love | ×8 for matching species with the **same** non-genderless gender, preserving the bug; reads actual party/enemy DVs and the species ROM gender ratio |
| Master | Original unconditional capture path, shown only when the surrounding throw situation is validated |

Ordinary HP scaling is `floor((3*maxHP - 2*HP)*modified_rate/(3*maxHP))`
only when its inputs fit the engine's arithmetic. The implementation follows
the actual byte operations: if 3*maxHP exceeds 255, both numerator components
are shifted twice, the denominator/current values become bytes, subtraction
wraps to a byte and the quotient is taken as a byte. A zero result becomes one.
This deliberately retains Crystal's **max-HP overflow bug**, rather than
substituting a modern formula. A zero denominator has no trusted estimate and
produces NA. Sleep/freeze then add ten (saturating at 255); poison, burn and
paralysis do not receive their intended +5 because of the original status bug.
The unused held catch-chance effect does not alter this pinned game's result.

The final eight-bit comparison accepts both less-than **and equality**.
The modeled probability is therefore `(final_rate+1)/256`, not final_rate/255
or final_rate/256. With rate 255, HP 20/20 and a Poké Ball, this is 86/256,
reported as 3359 basis points internally and `~33.5%` on screen. Ball or
status bonuses can saturate this comparison; Master has its separate path.

## Selection and uncertainty guards

Only Crystal's battle ball-list layout is enabled: pocket 1, settled list state
4, its exact menu pointer/bank, 5×8 quantity-list metadata and inventory pointer,
list border geometry, cursor/scroll/count bounds, selected item agreement with
both item/description registers, full rendered pack header and ▶ cursor.
Reordering, CANCEL, other pockets, confirmation menus, non-ball entries, zero
quantity, transitions, trainer fights and training screens hide the panel.
Stale `wCurItem` or the previously thrown ball is never enough on its own.

Tutorial/debug/contest battle kinds, unsupported species, mismatched original
enemy, corrupt HP/status, missing required reads or special-ball lookup failures
produce NA. The helper does not assume that six party members imply a free PC
box: Crystal's current box capacity needs a reliable banked SRAM mapping.
Until that mapping exists, **a full six-member party gives NA for every ball,
including Master**. Safari/Park/GS and special scripted ball flows are excluded.
Missing optional weight/gender data affects that ball's estimate alone.

Read-only access, exact ROM/backend gating, source-frame immutability, duplicate
frame cleanup and reset/load/unload behavior remain unchanged. The private
checksum-verified ROM copy introduced for training bonuses also supplies
bank-independent dex/base-data reads; no commercial data is bundled in source.

Gen 1/3 catch menus remain deliberately unsupported. Their existing battle
profiles do not validate their selected inventory ball or every required
condition. They require separate bag profiles and their own algorithms, rather
than reusing Crystal addresses/formulas. Their battle/training HUDs remain usable.

## Sources and address provenance

- [Original Crystal ball effect and all multipliers](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/engine/items/item_effects.asm)
- [BattlePack/BallsPocketMenuHeader](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/engine/items/pack.asm)
- [Scrolling cursor/item/description updates](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/engine/menus/scrolling_menu.asm)
- [Gender calculation](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/engine/pokemon/mon_stats.asm)
- [Fast Ball's flee list](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/data/wild/flee_mons.asm)

Addresses come from the existing pinned [Crystal 1.1 symbols](https://github.com/pret/pokecrystal/blob/87b0d7436e43c3717cfc416d38a99162191bb714/pokecrystal11.sym):
menu data `04:4ab7`; pocket CF65; list state CF63; border CF82..CF85;
list metadata CF92..CF97; cursor CFA9; scroll D0E4; reorder D0E3;
ball inventory D8D7/D8D8; current item D106; selection CF74; tilemap C4A0;
original enemy/player D204/D205; status D214; catch rate D22B; battle kind D230;
player level C639; active party slot D0D4; enemy DVs D20C;
dex pointer table `11:4378`; original weight bank table `03:6c4c`.
Banked ROM locations use the private 0x10000000+file-offset virtual read range.

## Validation and remaining acceptance

Automated tests cover all 499,500 valid HP/max-HP pairs through 999, inclusive
RNG comparisons, sleep/freeze and ignored status bonuses, overflow/zero divisor,
Level boundaries, every supported ball across all five cursor rows, scroll/CANCEL,
selected-item disagreement, quantity/menu/pocket/signature/read failures,
trainer/transition/source/capacity guards, original Fast/Moon/Heavy/Love behavior
and unsupported generation gating. Full battle-decoder integration is checked.
All three software formats preserve every pixel outside the panel, rebuild
percentage/ball/NA changes on NULL duplicate frames and erase the panel on
menu exit or option disable. Local ASan/UBSan passes for catch/options/renderer;
synthetic panel previews were visually inspected.

Real Gambatte integration runs the component over actual emulated RAM/pixels,
then restores test-host fixture writes before direct/proxy state-parity checks.
The production decoder does not write RAM and still rejects the original test
ROM's checksum. These fixtures are not an interactive Crystal playtest.

Interactive macOS acceptance remains pending: in a wild Crystal battle with
fewer than six party members, open PACK → BALL, highlight a ball and check the
panel; change highlight if another ball is available, then leave the list.
NextUI hardware acceptance and Gen 1/3 catch profiles also remain pending.

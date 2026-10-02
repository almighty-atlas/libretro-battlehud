# Suggested next features

Hidden Power type and power on the Gen 2/3 training page is now implemented;
see [details](hidden-power.md). Emerald nature bonuses and stored ability are
also implemented; see [details](nature-ability.md) and [issue #3](https://github.com/almighty-atlas/libretro-battlehud/issues/3).
Independent HUD preferences and compact/detail layouts are also implemented;
see [settings](hud-options.md). Their manual acceptance remains pending. The remaining items below are proposals. Priority reflects usefulness for
playing on a small handheld screen and the profiles currently available.

1. **Training progress**: EV total/remaining budget in Gen 3 (total is already
   displayed), per-stat effective contribution and optional gains since the last
   battle. Gen 1/2 need their own stat-experience/bonus formula, without modern caps.
2. **Battle type icons and effectiveness for Gen 1/3**: separate verified battle
   profiles and generation-correct charts. Gen 3 must account for abilities,
   immunities and special move rules before making definite predictions.
3. **Catch helper**: a compact catch-chance estimate for the chosen ball, current
   HP/status and generation-specific rules, with uncertainty clearly represented.
4. **Useful party details**: friendship (Gen 2/3), Pokérus/EV multiplier (Gen 3),
   shiny status and gender where relevant, with generation-specific definitions.
5. **More ROM profiles**: Gold/Silver, Yellow, Ruby/Sapphire and FireRed/LeafGreen,
   then German editions, each gated by exact checksum and tested menu layouts.

Suggested next step: verified battle support for Gen 1/3 ([issue #5](https://github.com/almighty-atlas/libretro-battlehud/issues/5)).
Expand edition/language profiles when there is a concrete ROM and frontend
available for acceptance testing.

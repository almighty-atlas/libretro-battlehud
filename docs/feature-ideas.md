# Suggested next features

Hidden Power type and power on the Gen 2/3 training page is now implemented;
see [details](hidden-power.md). Emerald nature bonuses and stored ability are
also implemented; see [details](nature-ability.md) and [issue #3](https://github.com/almighty-atlas/libretro-battlehud/issues/3).
Independent HUD preferences and compact/detail layouts are also implemented;
see [settings](hud-options.md). Gen 1/3 battle profiles are also implemented;
see [battle rules and limits](multigen-battles.md). Their manual acceptance remains pending. The remaining items below are proposals. Priority reflects usefulness for
playing on a small handheld screen and the profiles currently available.

1. **Catch helper**: a compact catch-chance estimate for the chosen ball, current
   HP/status and generation-specific rules, with uncertainty clearly represented.
2. **Useful party details**: friendship (Gen 2/3), Pokérus/EV multiplier (Gen 3),
   shiny status and gender where relevant, with generation-specific definitions.
3. **More ROM profiles**: Gold/Silver, Yellow, Ruby/Sapphire and FireRed/LeafGreen,
   then German editions, each gated by exact checksum and tested menu layouts.

Training progress is also implemented in PR #1; see [views, formulas and limits](training-progress.md).
Suggested next implementation: catch helper ([issue #7](https://github.com/almighty-atlas/libretro-battlehud/issues/7)).
Expand edition/language profiles when there is a concrete ROM and frontend
available for acceptance testing.

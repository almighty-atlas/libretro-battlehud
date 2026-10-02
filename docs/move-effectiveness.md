# Crystal FIGHT move hints

The normal battle menu and FIGHT show original 8x8 type silhouettes in 12x12
colored tiles instead of type names. In FIGHT, a 7x7 marker appears in the unused
rightmost interior tile beside each move. Move names, cursor, borders and PP
remain intact. Hints are limited to the recognized 160x144 Crystal layout.

| Marker | Meaning |
| --- | --- |
| Green up arrow | Super effective (2x or 4x) |
| Amber down arrow | Resisted (1/2x or 1/4x) |
| Blue-grey equals | Neutral damage effectiveness (1x) |
| Red cross | Type immunity (0x) |
| Grey dash | Status move; no damage multiplier |
| Grey hollow square | No PP or currently disabled |
| Amber question mark | Unknown or conditional move outcome |

This describes type effectiveness, not accuracy, damage amount, or a promise that
a move succeeds. Status moves are deliberately not treated as ordinary damage:
Growl can affect a Ghost despite being Normal. Status-specific immunity,
existing ailments, Protect, Substitute, weather, STAB and other success/damage
conditions are not inferred by these symbols.

The two current enemy types are combined, with identical types counted once.
Gen 2 rules retain Steel's Ghost/Dark resistances. Foresight removes Ghost's
Normal/Fighting immunities. Hidden Power's type uses the active battle DVs,
including after Transform. Fixed-damage and OHKO moves display neutral unless
type-immune. Counter, Mirror Coat and Bide display a question mark unless immune;
their damage depends on actions not yet taken. Metronome, Mirror Move and Sleep
Talk also remain unknown. Future Sight, Beat Up and Struggle bypass the type
chart in Crystal and display neutral. Status moves display a dash.

Battle RAM is read only. Slots are annotated only when all required reads succeed,
IDs are valid/contiguous and the move count matches the actual FIGHT row count.
Missing or stale data removes hints while retaining valid enemy icons. No type,
move, PP, or status data is written back. Save-state restoration and duplicate
frame composition use the same clean-frame invalidation rules as the original HUD.

## Pinned primary sources

Source revision: `pret/pokecrystal` at
`5beda23ffa505f62e1dad7e3d7c214d1737b3358`:

- [Move facts](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/data/moves/moves.asm)
- [Gen 2 type chart](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/data/types/type_matchups.asm)
- [MoveSelectionScreen, MoveInfoBox and disabled slot](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/engine/battle/core.asm)
- [Hidden Power calculation](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/engine/battle/hidden_power.asm)
- [Damage/effect command paths](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/data/moves/effects.asm)
- [Type checks and Foresight](https://github.com/pret/pokecrystal/blob/5beda23ffa505f62e1dad7e3d7c214d1737b3358/engine/battle/effect_commands.asm)

Rev. 1 symbols from `pokecrystal11.sym`, symbols commit
`87b0d7436e43c3717cfc416d38a99162191bb714`:
`wBattleMonMoves=C62E`, `wBattleMonDVs=C632`, `wBattleMonPP=C634`,
`wPlayerDisableCount=C675`, `wEnemySubStatus1=C66D` (IDENTIFIED bit 3).
The upper disable-count nibble is the one-based move slot and the lower nibble
is the remaining count. PP uses the low six bits, excluding PP Ups.

`src/gen2_move_data.h` contains numeric type/rule facts for all 251 moves and
110 non-neutral matchups; no ROM bytes, original graphics or game executable
are bundled. The icon silhouettes are hand-authored renderer assets.

## Validation

Decoder tests cover 4x/1/4x, cancellation, immunity, duplicate types, Steel's
Gen 2 resistances, Foresight, all 16 Hidden Power types, fixed/conditional moves,
PP/Disable and missing/stale move metadata. Pixel tests cover each marker in all
three formats, immutable source buffers, unchanged pixels outside overlays,
duplicate redraw/removal and submenu cleanup. A renderer-produced synthetic
FIGHT preview and type atlas were inspected; actual Crystal Mac acceptance of
the new icons and hints remains pending. Existing Mac save-state restoration
was confirmed before this extension. Physical H700 acceptance remains pending.

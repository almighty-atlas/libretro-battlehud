# Type HUD (M4)

Recognized Crystal Rev. 1 battles display one or two colored badges at the upper
right **in the normal FIGHT / PKMN / PACK / RUN main menu and in FIGHT move selection**. Types use original 8×8 pixel silhouettes, with a dark border and white ink. Identical types produce one badge. The
model is read at the core's video callback before composition, so the rendered
state belongs to that frame. Outside battle, in bag/party submenus and action text, and during
invalid/unavailable or start/switch/faint transitions, no badges are drawn.
The profile checks the menu-data pointer/bank plus all four RAM tilemap labels;
missing or mismatched data hides the badges without discarding the combatant.

At 160×144 each icon tile is 12×12 pixels, with two-pixel outer margins and a
two-pixel row gap. Smaller frames that cannot fit the icons pass through.
FIGHT adds a symbol beside each move; see [effectiveness rules](move-effectiveness.md).
0RGB1555, RGB565 and XRGB8888 are supported. Hardware frames, unknown formats and
malformed/oversized layouts pass through unchanged.

## Const source buffers and duplicates

The renderer owns a reusable clean-frame buffer and output buffer. It copies
visible row bytes only, respecting pitch without reading the final row's padding.
It does not modify a core frame or RAM. Clean-frame caching is restricted to
recognized games with HUD enabled; unknown games and disabled HUD retain pointer
and frame forwarding without allocation. Outside battle a fresh core frame is
forwarded directly, while the clean cache remains ready for a state change on a
NULL duplicate.

A NULL frame normally means the frontend retains its previous image. If the model
is unchanged, the renderer preserves that duplicate. If types or visibility
change, it recomposes the cached clean frame; battle end sends the clean frame so
old badges cannot remain visible. Layout changes invalidate the cache. On reset,
successful state restore, game load/unload or deinit the buffers are cleared,
preventing reuse of a previous timeline or game's image. Allocation failures fall
back to a clean frame. Each buffer is limited to 32 MiB; GB/GBC frames are much
smaller. The opt-in M1 marker remains a separate development test layer.

## Development switch and acceptance

Badges are enabled by default for the supported profile. Set
`LIBRETRO_BATTLEHUD_DISABLE_HUD=1` before launch to compare plain emulation;
`LIBRETRO_BATTLEHUD_DEBUG=1` independently controls terminal diagnostics.
There is no unsupported-ROM or profile override.

Unit tests exercise all formats/type icons, exact pixels, source preservation,
pixels outside badges, duplicate updates/removal, submenu visibility changes, layout failures and cleanup.
Real-Gambatte component tests feed actual emulator pixels and the synthetic
fixture's decoded model into the renderer, while the production ROM gate remains
closed for that original test ROM. The prior text-badge Mac tests and save-state restoration passed; the new
icon/hint appearance still needs interactive acceptance. Physical NextUI/H700 testing remains M5.

The same read-only compositor also draws the party
[DV/stat-experience table](training-stats.md) on the third stats page.

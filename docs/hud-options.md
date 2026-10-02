# HUD preferences

All displays default to enabled; the layout defaults to `detailed`. The proxy
appends these options to the real core's options, retaining backend keys, values,
defaults, translations and categories. Legacy variables, V1/V2 and both
international registration formats are supported. HUD descriptions use English.

| Option key | Label | Values / behavior |
| --- | --- | --- |
| `battlehud_types` | BattleHUD: Type icons | `enabled` / `disabled`; opponent badges in recognized battle menus |
| `battlehud_moves` | BattleHUD: Move effectiveness | `enabled` / `disabled`; independent FIGHT hints |
| `battlehud_training` | BattleHUD: Training values | `enabled` / `disabled`; DV/IV and training table |
| `battlehud_hidden_power` | BattleHUD: Hidden Power | `enabled` / `disabled`; independent Gen 2/3 type and power row |
| `battlehud_party_details` | BattleHUD: Nature and ability | `enabled` / `disabled`; Gen 3 name, stat arrows and ability |
| `battlehud_layout` | BattleHUD: Layout | `detailed` / `compact`; compact omits EV/stat-experience columns and their total/footer, retaining DV/IV; other toggles remain independent |

Options affect only supported ROM profiles and pages, including the separate
[Gen 1/3 battle profiles](multigen-battles.md). Turning off the table still allows Hidden Power and
Gen 3 nature/ability to be shown independently. Stat arrows belong to table
rows and require both training values and nature/ability to be enabled.
`LIBRETRO_BATTLEHUD_DISABLE_HUD=1` overrides every HUD preference.

In RetroArch, load the proxy and open **Quick Menu → Core Options**. Change the
`BattleHUD:` entries and resume; changes apply on the next software video
callback, including NULL duplicate frames. Saving options uses RetroArch's
normal core/game option controls. Backend options remain alongside HUD options.

In NextUI/MinArch, use the launcher's core-options screen when exposed by that
frontend/build; the same keys and values are registered through Libretro's
legacy API. Availability and saving depend on the frontend. There is no new HUD
hotkey or input interception. If the frontend offers no option menu, defaults
apply (the global environment disable remains available). NextUI hardware/menu
acceptance is pending; this behavior is not claimed as manually verified.

The compositor retains a clean source frame and compares both model and option
flags. A toggle/layout change rebuilds a duplicate frame from that clean source,
so disabled overlays are erased without modifying core-owned pixels, SRAM or
savestates. GET_VARIABLE_UPDATE remains available to the backend unchanged.

Automated checks cover all registration versions and translations, defaults,
key collisions, category return semantics, independent display regions,
compact rows, source immutability and erasure on duplicate frames in all three
software pixel formats. Real Gambatte/mGBA integration additionally compares
all backend option definitions before/after wrapping and video/audio/state parity.

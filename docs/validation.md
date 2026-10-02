# Validation status

## M0 interactive desktop test — passed (user reported)

On 2026-10-02 the user tested wrapper commit
`77613206bd3fec433fd6f3ac39c8abc3cf0ce882` on an Apple M4 MacBook using RetroArch
installed with Homebrew's `retroarch-metal` cask and Gambatte from Core Downloader.
The game was `Pokemon - Crystal Version (USA, Europe) (Rev 1).gbc`.
The ROM hash and the frontend/backend version strings were not captured in this
manual session; the filename alone is not a verified game profile identifier.

The user confirmed:

- game startup, visible picture and keyboard input through the first opponent
- save-state restoration to the saved battle point, with picture and sound continuing
- in-game saving, clean quit/relaunch and loading the save via Continue
- Gambatte color-correction option changes while gameplay continued

This satisfies the initial M0 desktop-test gate. It is a user-reported interactive
test, distinct from the independently reproducible real-Gambatte CI parity checks.
It does not establish NextUI/H700 device behavior or Crystal battle decoding.

## M1 video marker

M1 adds an opt-in 8×8 white rectangle at the top right of software frames.
The marker is a video-interception test, without Pokémon parsing or HUD text.
Set `LIBRETRO_BATTLEHUD_TEST_MARKER=1` before starting the frontend to enable it.
Without that setting the proxy preserves the original video pointer and bytes.

The renderer supports 0RGB1555, RGB565 and XRGB8888. It uses a reusable scratch
buffer because Libretro video data is const and belongs to the emulator core.
Only visible row bytes are copied; pitch is preserved. Invalid layouts, unknown
formats, allocation failures and hardware-frame sentinels pass through unchanged.
NULL duplicate frames pass through unchanged so the frontend retains the prior
marked frame. Tiny frames clip the marker safely. The buffer is freed on deinit.

CI checks renderer behavior and fake-backend integration, plus real Gambatte with
the marker enabled: every visible marked frame must contain the white rectangle;
video outside it, audio, SRAM and save-state behavior must match direct Gambatte.

Interactive M1 acceptance passed (user reported) on 2026-10-02 using commit
`7c0a1844f8e60f8552d9a5df1318af2f25892b13`: the user confirmed the white square
is visible in the running Crystal session on the same Apple M4 MacBook.
H700 validation remains pending.


## M2 read-only memory adapter

The wrapper captures `RETRO_ENVIRONMENT_SET_MEMORY_MAPS` independently of frontend
support, copies descriptors and address-space labels, and borrows only the core's
session-valid data pointers. CPU reads use an explicit address mapping; raw
`retro_get_memory_data` fallback uses memory ID plus region offset, with no guessed
CPU base address. Neither API exposes a mutable pointer to the decoder.

The initial adapter supports bounded, linear mappings with `disconnect == 0` in
the unnamed CPU address space, including Gambatte's $C000/$D000 RAM windows and
$FF80 zero page. Unsupported disconnected, implicit-length or mirrored addresses
fail closed rather than returning speculative data. Named spaces are retained
but unavailable through the CPU read API. This is deliberately not a general
Libretro bank/mirror normalizer. Limits are 256 descriptors and 1 MiB per read.

`battlehud_read_memory(address, destination, size)` and
`battlehud_read_region(id, offset, destination, size)` return false on unmapped,
unsupported, overflowing or out-of-range reads, leaving the destination unchanged.
Call on the emulation thread between frames. The caller must supply a writable,
non-overlapping destination buffer. Game load, failed load, unload and deinit
invalidate prior mappings; new announcements replace them. Backend pointers must
remain valid for the session as required by Libretro's descriptor contract.

Unit tests cover descriptor copying, offsets, selection masks, first-descriptor
precedence, crossing boundaries, unmapped/named spaces, unsupported mappings,
overflows and unchanged source/destination on failures. Fake-backend tests cover
frontend rejection and lifecycle invalidation. Real-Gambatte parity tests use an
original ROM writing $6D to CPU $C123; the normalized read is compared with the
backend SYSTEM_RAM byte at offset $123. SRAM, save-state restore, audio and video
parity are checked alongside it. No commercial ROM or Pokémon RAM decoding is
needed for this test. Crystal profile recognition and battle decoding remain M3.


## M3 Crystal decoder — interactive desktop acceptance passed (user reported)

ROM recognition uses SHA-1, with one compiled Crystal USA/Europe Rev. 1 profile.
A pure decoder consumes an abstract read-only memory callback. Unknown ROMs and
invalid/unavailable state yield an empty enemy model; normal emulator operation
continues. No final HUD is drawn at this milestone.

Automated coverage adds known SHA-1 vectors (including multi-block streaming and
file input) and battle fixtures for single/dual types, wild/trainer battles,
opponent changes, battle end, restore-shaped snapshots and malformed data.
The original GBC ROM integration test checks physical bank-1 reads and decodes a
literal Pidgey fixture through real Gambatte while confirming production profile
selection rejects the synthetic ROM. M3 brought the suite to 12 CTest cases.

On 2026-10-02 the user tested commit
`fe2f046119426f234fea83c9b1977dac96801949` on the same Apple M4 MacBook.
Manual `shasum` output confirmed the supported ROM SHA-1
`f2f52230b536214ef7c9924f483392993e226cfb`.

| Interactive case | Evidence |
|---|---|
| Wild / dual type | User identified Taubsi; log: `wild species=16 types=NORMAL/FLYING raw=00/02` |
| Single type | User identified Wiesor; log: `wild species=161 types=NORMAL raw=00/00` |
| Trainer | User identified Endivie; log: `trainer species=152 types=GRASS raw=16/16` |
| Enemy switch | Same trainer sequence changed from ID 16 / NORMAL-FLYING to ID 19 / NORMAL, separated by a transition |
| Battle end | Repeated `hidden (outside battle)` after the tested battles |
| Save-state restoration | User reported correct restoration after the guided save/end/restore test; exact restoration log not captured |

The switched trainer's Pokémon names (Taubsi/Rattfratz) are inferred from the IDs;
the user described the last sequence as probably a two-Pokémon trainer battle.
This is desktop acceptance from user-provided logs and observations. The frontend
and backend version strings and startup profile line were not captured. Physical
NextUI/H700 and link/mobile/special battles remain unverified.
[Reverse-engineering notes](reverse-engineering.md) preserve sources and limits.


## M4 type badges — interactive acceptance pending

The renderer draws one or two English type badges at the upper right for a valid
active Crystal battle at the normal battle main menu. It supports 0RGB1555, RGB565 and XRGB8888, samples the model
at the video callback and preserves the core's source buffer. Unknown games and
`LIBRETRO_BATTLEHUD_DISABLE_HUD=1` keep normal video forwarding.

The 13th CTest case covers actual border/background/glyph pixels, unchanged pixels
outside both badges, immutable source data, all type labels/formats, allocation
reuse, hidden/invalid states, duplicate-frame model changes and badge removal,
layout changes, hardware/unknown-format fallbacks and teardown. Exact source
allocations omit the final row's padding; AddressSanitizer/UndefinedBehaviorSanitizer
checks also cover the renderer. A software preview was inspected for readability.

Real-Gambatte integration decodes the original test-ROM fixture and runs the
renderer over captured emulator pixels. It checks a glyph, pixels outside the
badges, source preservation, model updates/removal on duplicates and buffer cleanup.
This is component integration; the synthetic ROM remains rejected by production
profile selection. The actual automatic Crystal HUD still needs the user's Mac
test for visibility, enemy changes, disappearance and save-state restoration.


### M4 initial visual test and main-menu refinement

On 2026-10-02 the user reported the M4 badges look good and supplied screenshots
from commit `e4340834f25334735833b3a18bc8fd46b229302b`: Rattata showed NORMAL and
Hoothoot showed NORMAL / FLYING. The screenshots also identify the backend as
`Gambatte v0.5.0-netlink d9d6cd0`. The initial single/dual badge visibility and
readability are confirmed; visual switch/end/restore acceptance remains pending.

The user requested visibility **only at the battle main menu**, including hiding
badges in the bag. The revised profile checks the live menu pointer/bank and all
four rendered tile labels. Decoder tests cover a valid menu, mismatched pointers
and banks, stale metadata, missing text/reads and the PKMN glyph expansion.
Renderer tests cover submenu entry/return on duplicate frames while preserving
the enemy model. Real-Gambatte synthetic fixture coverage includes the menu RAM
and renderer removal when `main_menu` becomes false. Actual bag/party/move screen
entry/return is pending the user's new Mac test.


### Confirmed bag visibility and FIGHT exception

On 2026-10-02 the user confirmed that commit
`fe959fd2f6b8b3798b272aa45cfe3638930d8ffc` shows badges in the main menu, hides them
in PACK and restores them when returning ("funktioniert perfekt"). The user then
requested that FIGHT move selection also show badges and waived a separate manual
test of that change. Automated decoder/renderer fixtures cover normal/disabled
move selection, PP-item/enemy menus, stale geometry/boxes and duplicate-frame
main/FIGHT/hidden transitions. FIGHT visibility is not recorded as manually tested.

## M5 NextUI H700 package — prepared, device acceptance pending

The isolated PBH package uses the verified custom `Emus/h700/PBH.pak` override and
ROM folder tag `(PBH)`. It keeps the frontend core filename `gambatte_libretro.so`,
loads the firmware's original core read-only via the backend environment override,
and isolates test saves/options/states under PBH. It replaces no firmware file.

A host test verifies the ZIP architecture/layout/modes, manifest, launcher path
quoting with spaces/apostrophes, backend/argument forwarding, missing-environment
failure and preservation of original GBC save/backend/ROM bytes. This is a mocked
launcher and packaging check, not a MinArch/device run. The suite has 14 CTest cases.
Physical SP startup, input/audio, HUD, saves and suspend/resume remain pending.

## Pixel icons and per-move hints

The user reconfirmed Mac save-state restoration. Previously accepted opponent
switch and disk save/restart checks were explicitly skipped instead of repeated.
Original 8x8 type silhouettes now replace text. FIGHT adds type-effectiveness
symbols in each move row, with status/conditional/unusable distinctions.
Fourteen host cases pass; decoder/renderer also pass ASan+UBSan. New feature
visual acceptance and physical H700 acceptance remain pending. See
[move-effectiveness details](move-effectiveness.md).

## Party DV/stat-experience extension

The user accepted the pixel icons/FIGHT appearance ("sieht sehr gut aus").
The third player-party stats page now displays DVs and raw Gen 2 stat experience
in the lower-left pane. Original right-hand stats remain untouched. Training
fixtures and pixel-boundary/duplicate tests pass, including ASan+UBSan.
Interactive acceptance of the new table and physical-device acceptance are
pending; see [training stats](training-stats.md).

## Issue #4: training progress

Local checks pass for all generation formulas, Gen 3 EV budget, conservative
party observation and raw/BON/GAIN/NA renderer switching. See
[coverage and pending interactive acceptance](training-progress.md#validation).
The configured test suite now contains 21 CTest cases; the CI also runs real
Gambatte/mGBA direct-versus-proxy integration on Linux and Apple Silicon.
The new training views have not yet been manually accepted on macOS or NextUI.

## Issue #7: initial Crystal catch helper

Local original-rule catch/menu/decoder/renderer tests and ASan/UBSan pass.
The configured suite now contains 22 CTest cases plus real Gambatte/mGBA
integration on Linux and native macOS. [Coverage and limits](catch-help.md#validation-and-remaining-acceptance)
separate automated fixtures from pending interactive Crystal/NextUI acceptance.
Gen 1/3 catch menus and full-party banked box-capacity reads are not implemented.

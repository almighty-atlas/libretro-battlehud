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


## M3 Crystal decoder — interactive acceptance pending

ROM recognition uses SHA-1, with one compiled Crystal USA/Europe Rev. 1 profile.
A pure decoder consumes an abstract read-only memory callback. Unknown ROMs and
invalid/unavailable state yield an empty enemy model; normal emulator operation
continues. No final HUD is drawn at this milestone.

Automated coverage adds known SHA-1 vectors (including multi-block streaming and
file input) and battle fixtures for single/dual types, wild/trainer battles,
opponent changes, battle end, restore-shaped snapshots and malformed data.
The original GBC ROM integration test checks physical bank-1 reads and decodes a
literal Pidgey fixture through real Gambatte while confirming production profile
selection rejects the synthetic ROM. The suite now contains 12 CTest cases.

These checks do not mean the six interactive M3 battle cases are passed. Needed
on the user's Mac: recognized hash, single/dual types, wild/trainer battle,
opponent switch, battle end, and restoration with no stale enemy model.
[Reverse-engineering notes](reverse-engineering.md) preserve sources and limits.

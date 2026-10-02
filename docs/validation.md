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

Interactive M1 acceptance remains pending: the user must see the rectangle while
Crystal runs normally on the Mac. H700 validation remains pending as well.

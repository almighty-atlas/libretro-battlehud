# M0: Libretro proxy

## Contract

M0 must behave like the backend core with no game-specific behavior.

The wrapper currently intercepts the callback setters only so it can place transparent forwarding callbacks between the real core and the frontend.

### Frontend → wrapper → backend

The standard Libretro entry points are forwarded, including:

- lifecycle and game loading
- system and AV information
- input configuration
- reset/run
- serialization
- cheats
- region
- memory data and size

### Backend → wrapper → frontend

These callbacks are transparently forwarded:

- environment
- video refresh
- audio sample
- audio batch
- input poll
- input state

M1 will modify only the video path. M2 will begin observing memory-map information in the environment path.

## Backend discovery

For development, set:

```sh
export LIBRETRO_BATTLEHUD_BACKEND=/absolute/path/to/gambatte_libretro.so
```

If unset, the wrapper looks beside itself for:

```text
gambatte_real_libretro.so  # Linux
gambatte_real_libretro.dylib  # macOS
```

This adjacent-file fallback is intended for later self-contained NextUI packaging.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

The CTest suite uses a fake Libretro backend and does not require ROMs or Gambatte.
It covers ordinary and adjacent-file loading, recovery after a missing backend,
missing symbols, incompatible API versions, and accidental self-loading. It also
checks padded frame bytes, NULL duplicate frames, input return values, audio
backpressure, callback removal, SRAM, save states, and retained-wrapper reinitialization.

CI additionally builds real Gambatte at pinned commit
`d9d6cd06382d1ced30de34d56d3609452323dab1` and compares direct execution with execution
through the wrapper for 120 frames. The integration test creates an original 32 KiB
GB test program; no commercial ROM or BIOS is downloaded. Visible video, audio,
input polls, SRAM and serialization size must match, and each core must restore SRAM
from a save state. Run it locally with an already built backend:

```sh
python3 tests/gambatte_integration.py build/pokemon_gambatte_libretro.so /absolute/path/to/gambatte_libretro.so
```

This headless check does not validate audible playback, physical controls, frontend
menus or device suspend/resume. See [NextUI analysis](nextui-analysis.md).

## M0 exit criteria

Before M1, validate with real Gambatte in the target frontend that:

- a GB/GBC ROM starts
- video, audio and input work
- save RAM is unchanged
- save states work
- core options still work
- shutdown is clean

No Pokémon-specific code belongs in M0.

## Loading and teardown behavior

The proxy rejects itself as a backend, requires all standard entry points and API
version 1, and returns safe empty/false results on load failure. It retains frontend
callback registrations so a later successful load or a reinitialization can replay
them. A NULL callback stays NULL at the backend. `retro_deinit` closes an already
loaded backend and does not attempt to load an unavailable backend during teardown.

The loader targets POSIX shared libraries with `.so` on Linux and `.dylib` on
macOS. See [Apple Silicon testing](macos-test.md) for a desktop M0 check. Windows
support and hardware-rendering backends are not validated.

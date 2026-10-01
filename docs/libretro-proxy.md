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
gambatte_real_libretro.so
```

This adjacent-file fallback is intended for later self-contained NextUI packaging.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

The test suite uses a fake Libretro backend and does not require ROMs or Gambatte.

## M0 exit criteria

Before M1, validate with real Gambatte that:

- a GB/GBC ROM starts
- video, audio and input work
- save RAM is unchanged
- save states work
- core options still work
- shutdown is clean

No Pokémon-specific code belongs in M0.

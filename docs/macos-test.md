# M0 test on an Apple Silicon Mac

The macOS build allows interactive M0 verification before an H700 handheld is
available. RetroArch is the desktop frontend; it does not emulate NextUI itself.
Passing this test does not establish H700 input, standby or performance behavior.

## Install the frontend and backend

```sh
brew install --cask retroarch-metal
open -a RetroArch
```

In RetroArch, select Online Updater → Core Downloader →
Nintendo - Game Boy / Color (Gambatte).

Use the native Apple Silicon frontend and core. A Linux `.so` artifact, even an
AArch64 one, cannot be loaded on macOS. Our macOS artifact is a Mach-O arm64
`pokemon_gambatte_libretro.dylib`.

## Obtain or build the wrapper

Download `battlehud-macos-arm64-m0` from a successful GitHub Actions run. Extract it
into a separate development folder. It contains only the wrapper, without a game
or Gambatte binary. CI tests the wrapper and independently built pinned Gambatte;
the Core Downloader's Gambatte build still needs the interactive check below.

Alternatively, build from the PR branch on the Mac:

```sh
brew install cmake
cmake -S . -B build-macos -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build build-macos --parallel
ctest --test-dir build-macos --output-on-failure
```

The wrapper looks beside itself for `gambatte_real_libretro.dylib` on macOS and
`gambatte_real_libretro.so` on Linux. An absolute `LIBRETRO_BATTLEHUD_BACKEND` path
overrides either default. Do not overwrite the Core Downloader's installed core.

## Locate the downloaded core

RetroArch's Settings → Directory → Cores shows its actual core directory.
Common locations can also be checked in Terminal:

```sh
find "$HOME/Library/Application Support/RetroArch" /Applications/RetroArch.app \
  -name gambatte_libretro.dylib -print 2>/dev/null
```

Check `file` on the wrapper and downloaded core to confirm both support arm64.

## Interactive validation

First launch a GB/GBC ROM normally using the installed Gambatte core. Confirm
visible video, audible audio, keyboard/controller input, in-game SRAM saving,
save-state load and one Gambatte core option change. Use separate test saves.

Then quit RetroArch and start it from Terminal so it inherits the backend path.
Replace the three absolute paths below with the actual locations:

```sh
LIBRETRO_BATTLEHUD_BACKEND="/absolute/path/to/gambatte_libretro.dylib" \
  /Applications/RetroArch.app/Contents/MacOS/RetroArch \
  -L "/absolute/path/to/pokemon_gambatte_libretro.dylib" \
  "/absolute/path/to/test-game.gbc"
```

Repeat the same checks through the wrapper, including quit/relaunch and loading
the saves/states produced in the direct test. Preserve the test logs and record
the RetroArch version, Gambatte version, ROM hash and wrapper commit. Verify the
actual app executable name if the installation differs from the path above.

M0 renders no HUD or marker. Seeing an unchanged game is the expected result.
After a successful interactive desktop test, M1 can be developed and checked on
the Mac while H700 device validation remains explicitly outstanding.

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

Download `battlehud-macos-arm64-m1` from a successful GitHub Actions run. Extract it
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

The default M0 mode renders no marker. To test M1, prefix the launch command with
`LIBRETRO_BATTLEHUD_TEST_MARKER=1` as well as the backend variable. Expect an 8×8
white rectangle in the top-right corner of the game image. There is no type HUD yet.
After a successful interactive desktop test, M1 can be developed and checked on
the Mac while H700 device validation remains explicitly outstanding.


## M3 terminal diagnostics

Use the `battlehud-macos-arm64-m3` artifact from the M3 CI run. Replace the previous
wrapper library after quitting RetroArch; keep the original Gambatte backend.
Then start with `LIBRETRO_BATTLEHUD_DEBUG=1` (the marker is optional):

```sh
LIBRETRO_BATTLEHUD_DEBUG=1 \
LIBRETRO_BATTLEHUD_BACKEND="$HOME/Library/Application Support/RetroArch/cores/gambatte_libretro.dylib" \
  /Applications/RetroArch.app/Contents/MacOS/RetroArch \
  -L "$HOME/Downloads/ABDM/Compressed/pokemon_gambatte_libretro.dylib" \
  "$HOME/Downloads/battlehud-test/Pokemon - Crystal Version (USA, Europe) (Rev 1).gbc"
```

Expected selection: `profile=pokemon-crystal-us-eu-rev1` with SHA-1
`f2f52230b536214ef7c9924f483392993e226cfb`. An unsupported hash disables decoding;
do not force a profile. Check the actual game file with:

```sh
shasum -a 1 "$HOME/Downloads/battlehud-test/Pokemon - Crystal Version (USA, Europe) (Rev 1).gbc"
```

At a wild Rattata, expect `wild species=19 types=NORMAL raw=00/00`.
A Pidgey should report `wild species=16 types=NORMAL/FLYING raw=00/02`.
Log entries appear on changes, not every frame. After battle, expect
`hidden (outside battle)`. Start/switch/faint transitions can temporarily hide the
model. Capture the profile line and observed enemy line first; trainer switching
and save-state restoration are later interactive checks. Type badges arrive in M4.

## M4 visual HUD test

Use `battlehud-macos-arm64-icons-moves`, quit RetroArch and replace the previous wrapper.
The supported Crystal profile now enables badges by default; no marker setting
is required. For quieter terminal output while testing:

```sh
LIBRETRO_BATTLEHUD_DEBUG=1 \
LIBRETRO_BATTLEHUD_BACKEND="$HOME/Library/Application Support/RetroArch/cores/gambatte_libretro.dylib" \
/Applications/RetroArch.app/Contents/MacOS/RetroArch \
  -L "$HOME/Downloads/ABDM/Compressed/pokemon_gambatte_libretro.dylib" \
  "$HOME/Downloads/battlehud-test/Pokemon - Crystal Version (USA, Europe) (Rev 1).gbc" \
  2>&1 | awk '/battlehud:/ { print; fflush() }'
```

First check a wild encounter: Taubsi should show NORMAL and FLYING, Wiesor NORMAL.
Then check that badges disappear after the battle. Subsequent acceptance checks
are trainer opponent changes and save-state restore. Capture a screenshot if the
badge placement obscures important game information. Use
`LIBRETRO_BATTLEHUD_DISABLE_HUD=1` in the launch environment for a plain-frame
comparison; diagnostics can stay enabled.


The refined M4 build shows badges in FIGHT / PKMN / PACK / RUN and FIGHT move selection. First verify:
open PACK → badges disappear, cancel back → badges reappear. Then repeat for PKMN. FIGHT move selection now keeps the badges visible. Diagnostics show `hidden (battle submenu)` while the
combatant remains available internally. Special contest/mobile menus are not
enabled by the normal-menu signature.

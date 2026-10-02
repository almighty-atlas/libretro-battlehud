# Game profiles

M3 currently supports only **Pokémon Crystal USA/Europe Rev. 1 (v1.1)**:

- profile ID: `pokemon-crystal-us-eu-rev1`
- SHA-1: `f2f52230b536214ef7c9924f483392993e226cfb`
- backend family: Gambatte (`retro_get_system_info().library_name == "Gambatte"`)

The wrapper hashes the ROM bytes supplied to `retro_load_game`, or reads the file
when the core uses full-path loading. File reads are streamed, capped at 32 MiB.
Names and headers do not select a profile. Unknown hashes, unreadable files,
other backend families and special/subsystem loads leave the decoder disabled;
emulation still passes through. There is no force-profile setting.

Profiles are compiled from `src/game_profile.c`, keeping RAM addresses separate
from the decoder and avoiding runtime YAML/JSON dependencies. Additional revisions
require their own verified hash and address provenance before being enabled.

The output is a `battle_state` value containing status, wild/trainer mode, species,
normalized types and raw type bytes. Gen 2 type codes are explicitly translated
into a generation-independent enum. Duplicate type bytes become one type;
Bird, Curse, gaps and out-of-range values are rejected. Types come from live
battle RAM, so type-changing battle effects are represented by the current values.

M4 adds type badges for a valid active model **at the normal battle main menu or FIGHT move selection**.
Bag, party, action text and other screens do not qualify; FIGHT move selection does.
The combatant remains decoded in submenus; visibility is separate. Terminal diagnostics remain available with
`LIBRETRO_BATTLEHUD_DEBUG=1`. The wrapper prints profile/hash/backend once and
battle-state changes rather than every frame. Outside battle, during start/switch
transitions, after enemy fainting and on invalid/unavailable data, the enemy model
is empty. Successful save-state restoration recomputes it; reset invalidates it
until the next frame; game load/unload/deinit clears it.

M3 interactive Crystal decoder acceptance passed on the user's Mac; see
[validation evidence](validation.md). M4 visual HUD acceptance is pending.

FIGHT move slots, PP, Disable, DVs and Foresight are decoded by the same
recognized Crystal profile; see [move-effectiveness provenance](move-effectiveness.md).

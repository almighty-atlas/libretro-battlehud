#include "game_profile.h"
#include <string.h>
/* pret/pokecrystal symbols commit 87b0d7436e43c3717cfc416d38a99162191bb714,
 * pokecrystal11.sym. Dxxx descriptors are physical WRAM bank 1 in Gambatte.
 * Full provenance and acceptance status: docs/reverse-engineering.md. */
static const struct game_profile crystal_rev1 = {
    .id = "pokemon-crystal-us-eu-rev1",
    .sha1 = "f2f52230b536214ef7c9924f483392993e226cfb",
    .battle_mode = 0xd22d, .battle_ended = 0xc734,
    .battle_starting = 0xd264, .enemy_switching = 0xc711,
    .enemy_species = 0xd206, .enemy_level = 0xd213,
    .enemy_hp = 0xd216, .enemy_max_hp = 0xd218,
    .enemy_type1 = 0xd224, .enemy_type2 = 0xd225,
    .menu_data_pointer = 0xcf86, .menu_data_bank = 0xcf8a,
    .main_menu_pointer = 0x4f34, .main_menu_bank = 0x09,
    /* wTilemap at C4A0, 20 columns. Text at (10,14)/(16,14)/(10,16)/(16,16).
     * PKMN expands to the two rendered glyph tiles E1/E2, not control byte 4A. */
    .main_menu_labels = {
        {0xc5c2, 5, {0x85, 0x88, 0x86, 0x87, 0x93}}, /* FIGHT */
        {0xc5c8, 2, {0xe1, 0xe2}},                   /* PKMN */
        {0xc5ea, 4, {0x8f, 0x80, 0x82, 0x8a}},       /* PACK */
        {0xc5f0, 3, {0x91, 0x94, 0x8d}}              /* RUN */
    },
    .move_menu_type = 0xd235, .move_geometry = 0xcfa1,
    .move_cursor_offsets = 0xcfa7,
    .move_origin_y = 13, .move_origin_x = 5, .move_max_rows = 4, .move_offset = 0x10,
    /* MoveInfoBox corners (0,8)/(10,8), move list bottom (4,17)/(19,17).
     * Corners remain valid for disabled moves; TYPE text does not. */
    .move_menu_labels = {
        {0xc540, 1, {0x79}}, {0xc54a, 1, {0x7b}},
        {0xc5f8, 1, {0x7d}}, {0xc607, 1, {0x7e}}
    }
};
const struct game_profile *game_profile_find(const char *sha1)
{
    return sha1 && !strcmp(sha1,crystal_rev1.sha1) ? &crystal_rev1 : NULL;
}

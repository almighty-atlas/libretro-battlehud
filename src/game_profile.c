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
    .enemy_type1 = 0xd224, .enemy_type2 = 0xd225
};
const struct game_profile *game_profile_find(const char *sha1)
{
    return sha1 && !strcmp(sha1,crystal_rev1.sha1) ? &crystal_rev1 : NULL;
}

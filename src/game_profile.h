#ifndef BATTLEHUD_GAME_PROFILE_H
#define BATTLEHUD_GAME_PROFILE_H
#include <stdint.h>
struct game_profile {
    const char *id, *sha1;
    uint16_t battle_mode, battle_ended, battle_starting, enemy_switching;
    uint16_t enemy_species, enemy_level, enemy_hp, enemy_max_hp, enemy_type1, enemy_type2;
};
const struct game_profile *game_profile_find(const char *sha1);
#endif

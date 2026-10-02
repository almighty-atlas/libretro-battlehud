#ifndef BATTLEHUD_GAME_PROFILE_H
#define BATTLEHUD_GAME_PROFILE_H
#include <stdint.h>
struct menu_label {
    uint16_t address;
    uint8_t size, tiles[5];
};
struct game_profile {
    const char *id, *sha1;
    uint16_t battle_mode, battle_ended, battle_starting, enemy_switching;
    uint16_t enemy_species, enemy_level, enemy_hp, enemy_max_hp, enemy_type1, enemy_type2;
    uint16_t menu_data_pointer, menu_data_bank, main_menu_pointer;
    uint8_t main_menu_bank;
    struct menu_label main_menu_labels[4];
    uint16_t move_menu_type, move_geometry, move_cursor_offsets;
    uint8_t move_origin_y, move_origin_x, move_max_rows, move_offset;
    struct menu_label move_menu_labels[4];
    uint16_t player_moves, player_dvs, player_pp, player_disable, enemy_substatus1;
};
const struct game_profile *game_profile_find(const char *sha1);
#endif

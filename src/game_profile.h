#ifndef BATTLEHUD_GAME_PROFILE_H
#define BATTLEHUD_GAME_PROFILE_H
#include <stdint.h>
struct menu_label {
    uint16_t address;
    uint8_t size, tiles[8];
};
struct gba_battle_profile {
    uint32_t callback,flags,mons,count,positions,absent,outcome;
    uint32_t controllers,exec,buffer,disable,bg_scroll;
    uint32_t action_input,move_input;
};
struct catch_profile {
    uint16_t menu_pointer,pocket,borders,geometry,cursor,scroll,balls,current_item,selection,tilemap,switch_item;
    uint16_t battle_type,original_enemy,original_player,catch_rate,enemy_status,player_level,player_slot,enemy_dvs;
    uint32_t dex_pointers,dex_banks;
};
struct game_profile {
    const char *id, *sha1, *backend_name;
    uint8_t generation;
    uint16_t battle_mode, battle_ended, battle_starting, enemy_switching;
    uint16_t enemy_species, enemy_level, enemy_hp, enemy_max_hp, enemy_type1, enemy_type2;
    uint16_t menu_data_pointer, menu_data_bank, main_menu_pointer;
    uint8_t main_menu_bank;
    struct menu_label main_menu_labels[4];
    uint16_t move_menu_type, move_geometry, move_cursor_offsets;
    uint8_t move_origin_y, move_origin_x, move_max_rows, move_offset;
    struct menu_label move_menu_labels[4];
    uint16_t player_moves, player_dvs, player_pp, player_disable, enemy_substatus1;
    uint16_t stats_flags, stats_state, mon_source, party_index, party_count, party_species;
    uint16_t party_base, temp_mon, stats_page_marker;
    struct menu_label stats_labels[5];
    uint32_t gba_party_count, gba_party_base, summary_pointer;
    uint32_t main_callback, summary_callback, tasks, input_task, palette_fade;
    uint32_t species_info, ability_names, mew_info;
    uint16_t enemy_identity,battle_type,escaped,move_rows;
    struct gba_battle_profile gba_battle;
    struct catch_profile catch_profile;
};
const struct game_profile *game_profile_find(const char *sha1);
#endif

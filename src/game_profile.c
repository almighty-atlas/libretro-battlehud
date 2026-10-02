#include "game_profile.h"
#include <string.h>
/* pret/pokecrystal symbols commit 87b0d7436e43c3717cfc416d38a99162191bb714,
 * pokecrystal11.sym. Dxxx descriptors are physical WRAM bank 1 in Gambatte.
 * Full provenance and acceptance status: docs/reverse-engineering.md. */
static const struct game_profile crystal_rev1 = {
    .id = "pokemon-crystal-us-eu-rev1", .backend_name = "Gambatte", .generation = 2,
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
    },
    .player_moves = 0xc62e, .player_dvs = 0xc632, .player_pp = 0xc634,
    .player_disable = 0xc675, .enemy_substatus1 = 0xc66d,
    .catch_profile={.menu_pointer=0x4ab7,.pocket=0xcf65,.borders=0xcf82,.geometry=0xcf92,
        .cursor=0xcfa9,.scroll=0xd0e4,.balls=0xd8d7,.current_item=0xd106,.selection=0xcf74,.tilemap=0xc4a0,.switch_item=0xd0e3,
        .battle_type=0xd230,.original_enemy=0xd204,.original_player=0xd205,.catch_rate=0xd22b,
        .enemy_status=0xd214,.player_level=0xc639,.player_slot=0xd0d4,.enemy_dvs=0xd20c,
        .dex_pointers=0x10044378,.dex_banks=0x1000ec4c},
    .species_info=0x10051424,
    .stats_flags = 0xcf64, .stats_state = 0xcf63, .mon_source = 0xcf5f,
    .party_index = 0xd109, .party_count = 0xdcd7, .party_species = 0xd108,
    .party_base = 0xdcdf, .temp_mon = 0xd10e, .stats_page_marker = 0xc515,
    .stats_labels = {
        {0xc54b,6,{0x80,0x93,0x93,0x80,0x82,0x8a}}, /* ATTACK */
        {0xc573,7,{0x83,0x84,0x85,0x84,0x8d,0x92,0x84}}, /* DEFENSE */
        {0xc59b,8,{0x92,0x8f,0x82,0x8b,0xe8,0x80,0x93,0x8a}}, /* SPCL.ATK */
        {0xc5c3,8,{0x92,0x8f,0x82,0x8b,0xe8,0x83,0x84,0x85}}, /* SPCL.DEF */
        {0xc5eb,5,{0x92,0x8f,0x84,0x84,0x83}} /* SPEED */
    }
};
/* pret/pokered symbols 3f618d59edf43918f48f5e558c34e04cb2fc5619: same WRAM in both editions. */
#define KANTO_PROFILE(name, digest) { \
    .id=name, .sha1=digest, .backend_name="Gambatte", .generation=1, \
    .battle_mode=0xd057, .battle_type=0xd05a, .escaped=0xd078, .battle_starting=0xd11d, \
    .enemy_species=0xcfe5, .enemy_identity=0xcfd8, .enemy_level=0xcff3, \
    .enemy_hp=0xcfe6, .enemy_max_hp=0xcff4, .enemy_type1=0xcfea, .enemy_type2=0xcfeb, \
    .menu_data_pointer=0xd125, .main_menu_pointer=0x0b, .move_geometry=0xcc24, \
    .move_menu_type=0xccdb, .move_rows=0xcd6c, .player_moves=0xd01c, \
    .player_pp=0xd02d, .player_disable=0xd06d, \
    .main_menu_labels={ {0xc4c2,5,{0x85,0x88,0x86,0x87,0x93}}, {0xc4c8,2,{0xe1,0xe2}}, \
        {0xc4ea,4,{0x88,0x93,0x84,0x8c}}, {0xc4f0,3,{0x91,0x94,0x8d}} }, \
    .move_menu_labels={ {0xc494,1,{0x7a}}, {0xc49a,1,{0x7e}}, \
        {0xc4f8,1,{0x7d}}, {0xc507,1,{0x7e}} }, \
    .mon_source=0xcc49, .party_index=0xcf92, .party_count=0xd163, \
    .party_base=0xd16b, .temp_mon=0xcf98, \
    .species_info=0x100383de, .mew_info=0x1000425b, \
    .stats_labels={ \
        {0xc455,6,{0x80,0x93,0x93,0x80,0x82,0x8a}}, \
        {0xc47d,7,{0x83,0x84,0x85,0x84,0x8d,0x92,0x84}}, \
        {0xc4a5,5,{0x92,0x8f,0x84,0x84,0x83}}, \
        {0xc4cd,7,{0x92,0x8f,0x84,0x82,0x88,0x80,0x8b}}, \
        {0xc45e,4,{0x93,0x98,0x8f,0x84}} \
    } \
}
/* TYPE1 label starts at (10,9): C3A0+190 = C45E; checked below separately. */
static const struct game_profile red = KANTO_PROFILE("pokemon-red-us", "ea9bcae617fdf159b045185467ae58b2e4a48b9a");
static const struct game_profile blue = KANTO_PROFILE("pokemon-blue-us", "d7037c83e1ae5b39bde3c30787637ba1d4c48ce2");
/* pret/pokeemerald symbols dba968c67d85caf9595abe12a51ff739d4dc5937.
 * Function pointers include the Thumb bit. Battle and summary decoders
 * require their own settled input callbacks. */
static const struct game_profile emerald = {
    .id="pokemon-emerald-us", .sha1="f3ae088181bf583e55daf962a92bb46f4f1d07b7",
    .backend_name="mGBA", .generation=3,
    .gba_party_count=0x020244e9, .gba_party_base=0x020244ec,
    .summary_pointer=0x0203cf1c, .main_callback=0x030022c4,
    .summary_callback=0x081bfab5, .tasks=0x03005e00,
    .input_task=0x081c0511, .palette_fade=0x02037fd4,
    .species_info=0x083203cc, .ability_names=0x0831b6db,
    .gba_battle={ .callback=0x08038421, .flags=0x02022fec, .mons=0x02024084,
        .count=0x0202406c, .positions=0x02024076, .absent=0x02024210, .outcome=0x0202433a,
        .controllers=0x03005d60, .exec=0x02024068, .buffer=0x02023064, .disable=0x020242bc,
        .bg_scroll=0x02022e14, .action_input=0x08057589, .move_input=0x08057bfd }
};
const struct game_profile *game_profile_find(const char *sha1)
{
    const struct game_profile *profiles[]={&crystal_rev1,&red,&blue,&emerald};
    for(unsigned i=0;sha1 && i<sizeof(profiles)/sizeof(*profiles);i++)
        if(!strcmp(sha1,profiles[i]->sha1)) return profiles[i];
    return NULL;
}

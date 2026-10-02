#ifdef NDEBUG
#undef NDEBUG
#endif
#include "battle_decoder.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
/* Independent literal fixtures based on the pinned pokecrystal11 symbols. */
static unsigned char ram[0x2000];
static bool missing;
static size_t missing_address;
static bool read_ram(void *ctx,size_t address,void *out,size_t size)
{
    (void)ctx;
    if(missing || address==missing_address || address<0xc000 || address>=0xe000 || size>0xe000-address) return false;
    memcpy(out,ram+address-0xc000,size); return true;
}
static void put(size_t address,unsigned char byte) { ram[address-0xc000]=byte; }
static void enemy(unsigned char species,unsigned char type1,unsigned char type2)
{
    put(0xd206,species); put(0xd213,5);
    put(0xd216,0); put(0xd217,20); put(0xd218,0); put(0xd219,20);
    put(0xd224,type1); put(0xd225,type2);
}
static void menu(void)
{
    put(0xcf86,0x34); put(0xcf87,0x4f); put(0xcf8a,9);
    const uint8_t fight[]={0x85,0x88,0x86,0x87,0x93}, pkmn[]={0xe1,0xe2};
    const uint8_t pack[]={0x8f,0x80,0x82,0x8a}, run[]={0x91,0x94,0x8d};
    memcpy(ram+0xc5c2-0xc000,fight,sizeof(fight));
    memcpy(ram+0xc5c8-0xc000,pkmn,sizeof(pkmn));
    memcpy(ram+0xc5ea-0xc000,pack,sizeof(pack));
    memcpy(ram+0xc5f0-0xc000,run,sizeof(run));
}
int main(void)
{
    const struct game_profile *p=game_profile_find("f2f52230b536214ef7c9924f483392993e226cfb");
    assert(p && !strcmp(p->id,"pokemon-crystal-us-eu-rev1"));
    assert(!game_profile_find(NULL)); assert(!game_profile_find("Pokemon Crystal.gbc"));
    assert(!game_profile_find("f4cd194bdee0d04ca4eac29e09b8e4e9d818c133")); /* Rev 0 */
    assert(!game_profile_find("f2f52230b536214ef7c9924f483392993e226cfa"));
    const unsigned char valid[]={0,1,2,3,4,5,7,8,9,20,21,22,23,24,25,26,27};
    const enum pokemon_type expected[]={TYPE_NORMAL,TYPE_FIGHTING,TYPE_FLYING,TYPE_POISON,
        TYPE_GROUND,TYPE_ROCK,TYPE_BUG,TYPE_GHOST,TYPE_STEEL,TYPE_FIRE,TYPE_WATER,
        TYPE_GRASS,TYPE_ELECTRIC,TYPE_PSYCHIC,TYPE_ICE,TYPE_DRAGON,TYPE_DARK};
    for(unsigned raw=0;raw<256;raw++) {
        enum pokemon_type type=TYPE_FAIRY; bool found=false;
        for(unsigned i=0;i<sizeof(valid);i++) if(raw==valid[i]) {
            found=true; assert(gen2_type_decode(raw,&type) && type==expected[i]);
        }
        if(!found) assert(!gen2_type_decode(raw,&type) && type==TYPE_FAIRY);
    }
    assert(!gen2_type_decode(0,NULL));
    assert(!strcmp(pokemon_type_name(TYPE_GHOST),"GHOST"));
    struct battle_state s=battle_decode(NULL,read_ram,NULL);
    assert(s.status==BATTLE_UNSUPPORTED && !s.species);
    s=battle_decode(p,read_ram,NULL); assert(s.status==BATTLE_OUTSIDE);
    /* Wild Rattata, single type. */
    put(0xd22d,1); enemy(19,0,0); menu();
    s=battle_decode(p,read_ram,NULL);
    assert(s.status==BATTLE_ACTIVE && s.mode==1 && s.species==19 &&
           s.type1==TYPE_NORMAL && s.type2==TYPE_NONE && s.raw_type1==0);
    assert(s.main_menu);
    struct battle_state previous=s;
    /* Bag/party/move menus: preserve opponent, independently hide the HUD. */
    put(0xcf86,0x00);
    s=battle_decode(p,read_ram,NULL);
    assert(s.status==BATTLE_ACTIVE && s.species==19 && !s.main_menu);
    assert(!battle_state_equal(&previous,&s));
    menu(); put(0xcf8a,10); /* Same pointer in a different ROM bank is not the menu. */
    s=battle_decode(p,read_ram,NULL); assert(!s.main_menu);
    menu(); put(0xc5c2,0); /* Stale header without the actual main-menu tile text. */
    s=battle_decode(p,read_ram,NULL); assert(!s.main_menu);
    menu(); put(0xc5c8,0x4a); /* Raw control character is not rendered PK/MN. */
    s=battle_decode(p,read_ram,NULL); assert(!s.main_menu);
    for(unsigned i=0;i<4;i++) {
        menu(); const size_t addresses[]={0xc5c2,0xc5c8,0xc5ea,0xc5f0};
        put(addresses[i],0); s=battle_decode(p,read_ram,NULL); assert(!s.main_menu);
    }
    menu(); missing_address=0xc5ea;
    s=battle_decode(p,read_ram,NULL); assert(s.status==BATTLE_ACTIVE && !s.main_menu);
    missing_address=0; s=battle_decode(p,read_ram,NULL); assert(s.main_menu);
    previous=s;
    assert(battle_state_equal(&previous,&s));
    /* Trainer Geodude followed by Zubat: no species/type cache. */
    put(0xd22d,2); enemy(74,5,4);
    s=battle_decode(p,read_ram,NULL);
    assert(s.status==BATTLE_ACTIVE && s.mode==2 && s.species==74 &&
           s.type1==TYPE_ROCK && s.type2==TYPE_GROUND);
    assert(!battle_state_equal(&previous,&s));
    put(0xc711,1); s=battle_decode(p,read_ram,NULL);
    assert(s.status==BATTLE_TRANSITION && !s.species && !s.type1);
    enemy(41,3,2); put(0xc711,0);
    s=battle_decode(p,read_ram,NULL);
    assert(s.status==BATTLE_ACTIVE && s.species==41 && s.type1==TYPE_POISON && s.type2==TYPE_FLYING);
    /* Types reflect current battle RAM, including type-changing moves. */
    put(0xd224,20); put(0xd225,20);
    s=battle_decode(p,read_ram,NULL); assert(s.type1==TYPE_FIRE && s.type2==TYPE_NONE);
    put(0xd264,1); s=battle_decode(p,read_ram,NULL); assert(s.status==BATTLE_TRANSITION);
    put(0xd264,0); put(0xd217,0);
    s=battle_decode(p,read_ram,NULL); assert(s.status==BATTLE_TRANSITION && !s.species);
    enemy(92,8,3); put(0xc734,1);
    s=battle_decode(p,read_ram,NULL); assert(s.status==BATTLE_OUTSIDE && !s.species);
    put(0xc734,0); put(0xd22d,0);
    s=battle_decode(p,read_ram,NULL); assert(s.status==BATTLE_OUTSIDE && !s.species);
    /* Restored battle snapshot is decoded immediately, without cached opponent. */
    put(0xd22d,1); s=battle_decode(p,read_ram,NULL);
    assert(s.status==BATTLE_ACTIVE && s.species==92 && s.type1==TYPE_GHOST && s.type2==TYPE_POISON);
    put(0xd224,6); s=battle_decode(p,read_ram,NULL); assert(s.status==BATTLE_INVALID && !s.type1);
    enemy(252,0,0); s=battle_decode(p,read_ram,NULL); assert(s.status==BATTLE_INVALID);
    enemy(19,0,0); put(0xd213,101); s=battle_decode(p,read_ram,NULL); assert(s.status==BATTLE_INVALID);
    enemy(19,0,0); put(0xd217,21); s=battle_decode(p,read_ram,NULL); assert(s.status==BATTLE_INVALID);
    put(0xd22d,3); s=battle_decode(p,read_ram,NULL); assert(s.status==BATTLE_INVALID);
    missing=true; s=battle_decode(p,read_ram,NULL); assert(s.status==BATTLE_UNAVAILABLE && !s.species);
    s=battle_decode(p,NULL,NULL); assert(s.status==BATTLE_UNAVAILABLE);
    puts("M3 single/dual types, wild/trainer, switching, end and invalid-data fixtures passed");
    return 0;
}

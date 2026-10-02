#ifdef NDEBUG
#undef NDEBUG
#endif
#include "battle_decoder.h"
#include "multigen_battle.h"
#include "multigen_effectiveness.h"
#include "move_effectiveness.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t gb[8192],ewram[0x40000],iwram[0x8000];
static size_t missing;
static uint8_t *at(size_t a,size_t n)
{
    if(a>=0xc000 && a<0xe000 && n<=0xe000-a)return gb+a-0xc000;
    if(a>=0x02000000 && a<0x02040000 && n<=0x02040000-a)return ewram+a-0x02000000;
    if(a>=0x03000000 && a<0x03008000 && n<=0x03008000-a)return iwram+a-0x03000000;
    return NULL;
}
static bool read_memory(void *c,size_t a,void *out,size_t n)
{
    (void)c;uint8_t *b=at(a,n);if(!b || (missing>=a && missing-a<n))return false;
    memcpy(out,b,n);return true;
}
static void byte(size_t a,unsigned v){*at(a,1)=(uint8_t)v;}
static void le16(size_t a,unsigned v){byte(a,v);byte(a+1,v>>8);}
static void le32(size_t a,uint32_t v){for(unsigned i=0;i<4;i++)byte(a+i,v>>(8*i));}
static void tiles(size_t a,const uint8_t *t,size_t n){memcpy(at(a,n),t,n);}
static void reset(void){memset(gb,0,sizeof(gb));memset(ewram,0,sizeof(ewram));memset(iwram,0,sizeof(iwram));missing=0;}
static const struct game_profile *profile(unsigned gen)
{return game_profile_find(gen==1?"ea9bcae617fdf159b045185467ae58b2e4a48b9a":"f3ae088181bf583e55daf962a92bb46f4f1d07b7");}
static struct battle_state decode(unsigned gen){return battle_decode(profile(gen),read_memory,NULL);}
static void kanto_fixture(bool fight)
{
    reset();byte(0xd057,1);byte(0xcfe5,36);byte(0xcfd8,36); /* internal Pidgey -> Dex 16 */
    byte(0xcff3,5);byte(0xcfe7,20);byte(0xcff5,20);byte(0xcfea,0);byte(0xcfeb,2);
    if(!fight){
        byte(0xd125,11);byte(0xcc24,14);byte(0xcc25,9);byte(0xcc28,1);
        const uint8_t a[]={0x85,0x88,0x86,0x87,0x93},b[]={0xe1,0xe2},c[]={0x88,0x93,0x84,0x8c},d[]={0x91,0x94,0x8d};
        tiles(0xc4c2,a,5);tiles(0xc4c8,b,2);tiles(0xc4ea,c,4);tiles(0xc4f0,d,3);
    }else{
        byte(0xcc24,12);byte(0xcc25,5);byte(0xcd6c,3);byte(0xcc28,5);
        byte(0xc494,0x7a);byte(0xc49a,0x7e);byte(0xc4f8,0x7d);byte(0xc507,0x7e);
        const uint8_t moves[]={85,33,68,45},pp[]={15,35,20,40};tiles(0xd01c,moves,4);tiles(0xd02d,pp,4);
    }
}
static void kanto_tests(void)
{
    kanto_fixture(false);struct battle_state s=decode(1);
    assert(s.status==BATTLE_ACTIVE && s.generation==1 && s.mode==1 && s.species==16 && s.type1==TYPE_NORMAL && s.type2==TYPE_FLYING && s.main_menu && !s.fight_menu);
    const struct game_profile *blue=game_profile_find("d7037c83e1ae5b39bde3c30787637ba1d4c48ce2");
    struct battle_state other=battle_decode(blue,read_memory,NULL);assert(battle_state_equal(&s,&other));
    byte(0xd057,2);assert(decode(1).mode==2);byte(0xcc25,15);assert(decode(1).main_menu);
    byte(0xc4ea,0);assert(!decode(1).main_menu); /* bag redraw with stale menu metadata */
    kanto_fixture(true);s=decode(1);assert(s.fight_menu && !s.main_menu);
    assert(s.moves[0]==85 && s.effectiveness[0]==MOVE_SUPER && s.effectiveness[1]==MOVE_NEUTRAL && s.effectiveness[2]==MOVE_UNKNOWN && s.effectiveness[3]==MOVE_STATUS);
    byte(0xd02d,0);byte(0xd06d,0x25);s=decode(1);assert(s.effectiveness[0]==MOVE_UNUSABLE && s.effectiveness[1]==MOVE_UNUSABLE);
    byte(0xccdb,1);assert(!decode(1).fight_menu);byte(0xccdb,0);byte(0xc507,0);assert(!decode(1).fight_menu);
    kanto_fixture(true);byte(0xcd6c,2);byte(0xcc28,4);assert(!decode(1).moves[0]);
    kanto_fixture(false);byte(0xcfe5,165);byte(0xcfd8,165);assert(decode(1).species==19); /* Rattata switch */
    byte(0xcfd8,36);assert(decode(1).status==BATTLE_TRANSITION);
    kanto_fixture(false);byte(0xcfe7,0);assert(decode(1).status==BATTLE_TRANSITION);
    kanto_fixture(false);byte(0xd11d,1);assert(decode(1).status==BATTLE_TRANSITION);
    kanto_fixture(false);byte(0xd078,1);assert(decode(1).status==BATTLE_OUTSIDE);
    kanto_fixture(false);byte(0xd057,0);assert(decode(1).status==BATTLE_OUTSIDE);
    kanto_fixture(false);byte(0xd05a,2);assert(decode(1).status==BATTLE_TRANSITION); /* Safari */
    kanto_fixture(false);byte(0xcfe5,31);byte(0xcfd8,31);assert(decode(1).status==BATTLE_INVALID); /* MissingNo */
    kanto_fixture(false);byte(0xcfea,9);assert(decode(1).status==BATTLE_INVALID);
    kanto_fixture(false);byte(0xcff5,19);assert(decode(1).status==BATTLE_INVALID);
    kanto_fixture(false);missing=0xcfe6;assert(decode(1).status==BATTLE_UNAVAILABLE);
}
static void mon(unsigned i,unsigned species,unsigned type1,unsigned type2,unsigned ability)
{
    size_t a=0x02024084+i*88;le16(a,species);byte(a+42,12);le16(a+40,30);le16(a+44,35);
    byte(a+33,type1);byte(a+34,type2);byte(a+32,ability);
}
static void selection(unsigned player)
{
    size_t a=0x02024084+player*88,b=0x02023064+player*512;
    byte(b,20);memcpy(at(b+4,8),at(a+12,8),8);memcpy(at(b+12,4),at(a+36,4),4);
    memcpy(at(b+20,2),at(a,2),2);memcpy(at(b+22,2),at(a+33,2),2);
}
static void emerald_fixture(bool fight)
{
    reset();le32(0x030022c4,0x08038421);le32(0x02022fec,4);byte(0x0202406c,2);
    byte(0x02024076,0);byte(0x02024077,1);le32(0x02024068,1);
    le16(0x02022e16,fight?320:160);le32(0x03005d60,fight?0x08057bfd:0x08057589);
    byte(0x02023064,fight?20:18);mon(0,277,12,12,65);mon(1,16,0,2,51);
    const unsigned moves[]={85,89,45,237};
    for(unsigned i=0;i<4;i++){le16(0x02024084+12+i*2,moves[i]);byte(0x02024084+36+i,15);}
    le32(0x02024084+20,0);if(fight)selection(0);
}
static void emerald_tests(void)
{
    emerald_fixture(false);struct battle_state s=decode(3);
    assert(s.status==BATTLE_ACTIVE && s.generation==3 && s.main_menu && !s.fight_menu && s.species==16 && s.type2==TYPE_FLYING);
    le32(0x02022fec,12);assert(decode(3).mode==2);
    emerald_fixture(true);s=decode(3);assert(s.fight_menu && !s.main_menu && !s.ambiguous_target);
    assert(s.moves[0]==85 && s.effectiveness[0]==MOVE_SUPER && s.effectiveness[1]==MOVE_IMMUNE && s.effectiveness[2]==MOVE_STATUS && s.effectiveness[3]==MOVE_NEUTRAL);
    byte(0x02024084+88+33,10);byte(0x02024084+88+34,10);assert(decode(3).effectiveness[3]==MOVE_NEUTRAL); /* HP Fighting vs Fire */
    /* Return to an unambiguous Flying foe, then test actual battle ability changes. */
    emerald_fixture(true);byte(0x02024084+88+32,10);assert(decode(3).effectiveness[0]==MOVE_IMMUNE);
    byte(0x02024084+88+32,26);byte(0x02024084+88+34,0);assert(decode(3).effectiveness[1]==MOVE_IMMUNE);
    byte(0x02024084+88+32,0);assert(decode(3).effectiveness[1]==MOVE_NEUTRAL);
    byte(0x02024084+36,0);selection(0);assert(decode(3).effectiveness[0]==MOVE_UNUSABLE);
    le16(0x020242bc+4,89);byte(0x020242bc+11,3);assert(decode(3).effectiveness[1]==MOVE_UNUSABLE);
    emerald_fixture(true);le16(0x02024084+12,354);selection(0);s=decode(3);assert(s.moves[0]==354 && s.effectiveness[0]==MOVE_NEUTRAL); /* no truncation */
    byte(0x02023064+12,7);assert(!decode(3).moves[0]); /* inconsistent list snapshot */
    emerald_fixture(false);le32(0x03005d60,0x08057588);assert(decode(3).status==BATTLE_TRANSITION); /* missing Thumb bit */
    emerald_fixture(false);byte(0x02023064,21);assert(decode(3).status==BATTLE_TRANSITION); /* Bag */
    emerald_fixture(false);le16(0x02022e16,320);assert(decode(3).status==BATTLE_TRANSITION);
    emerald_fixture(false);byte(0x02037fdb,128);assert(decode(3).status==BATTLE_TRANSITION);
    emerald_fixture(false);byte(0x0202433a,1);assert(decode(3).status==BATTLE_OUTSIDE);
    emerald_fixture(false);le32(0x030022c4,0);assert(decode(3).status==BATTLE_OUTSIDE);
    emerald_fixture(false);le32(0x02024068,0);assert(decode(3).status==BATTLE_TRANSITION);
    emerald_fixture(false);byte(0x02024077,0);assert(decode(3).status==BATTLE_INVALID);
    emerald_fixture(false);le16(0x02024084+88+40,0);assert(decode(3).status==BATTLE_TRANSITION);
    emerald_fixture(false);byte(0x02024084+88+32,78);assert(decode(3).status==BATTLE_TRANSITION);
    emerald_fixture(false);missing=0x02024084+88+33;assert(decode(3).status==BATTLE_UNAVAILABLE);
    emerald_fixture(true);le32(0x02022fec,5);byte(0x0202406c,4);byte(0x02024078,2);byte(0x02024079,3);
    mon(2,278,12,12,65);mon(3,19,0,0,50);s=decode(3);assert(s.fight_menu && s.ambiguous_target && s.effectiveness[0]==MOVE_UNKNOWN);
    byte(0x02024210,8);s=decode(3);assert(!s.ambiguous_target && s.effectiveness[0]==MOVE_SUPER);
    byte(0x02024210,2);s=decode(3);assert(s.species==19 && s.effectiveness[0]==MOVE_NEUTRAL);
    /* Position mapping, not a hard-coded enemy index. */
    emerald_fixture(true);uint8_t m[88];memcpy(m,at(0x02024084,88),88);memcpy(at(0x02024084,88),at(0x020240dc,88),88);memcpy(at(0x020240dc,88),m,88);
    byte(0x02024076,1);byte(0x02024077,0);le32(0x02024068,2);le32(0x03005d64,0x08057bfd);selection(1);s=decode(3);assert(s.species==16 && s.moves[0]==85 && s.effectiveness[0]==MOVE_SUPER);
}
static void rules(void)
{
    /* All single/dual combinations: Gen 1 differs from Gen 3 in four factual pairs. */
    for(enum pokemon_type a=TYPE_NORMAL;a<=TYPE_DRAGON;a++)for(enum pokemon_type d=TYPE_NORMAL;d<=TYPE_DRAGON;d++){
        int expected=gen3_type_factor(a,d,TYPE_NONE,false);
        if(a==TYPE_GHOST && d==TYPE_PSYCHIC)expected=0;
        if(a==TYPE_ICE && d==TYPE_FIRE)expected=4;
        if((a==TYPE_POISON && d==TYPE_BUG) || (a==TYPE_BUG && d==TYPE_POISON))expected=8;
        assert(gen1_type_factor(a,d,TYPE_NONE)==expected);
        for(enum pokemon_type e=TYPE_NORMAL;e<=TYPE_DRAGON;e++)assert(gen1_type_factor(a,d,e)==gen1_type_factor(a,d,TYPE_NONE)*(d==e?4:gen1_type_factor(a,e,TYPE_NONE))/4);
    }
    for(enum pokemon_type a=TYPE_NORMAL;a<=TYPE_STEEL;a++)for(enum pokemon_type d=TYPE_NORMAL;d<=TYPE_STEEL;d++)for(enum pokemon_type e=TYPE_NONE;e<=TYPE_STEEL;e++)for(unsigned id=0;id<2;id++)
        assert(gen3_type_factor(a,d,e,id)!=-1 && gen3_type_factor(a,d,e,id)==gen2_type_factor(a,d,e,id));
    assert(gen1_move_effectiveness(2,TYPE_ROCK,TYPE_NONE)==MOVE_RESISTED); /* Karate Chop Normal */
    assert(gen1_move_effectiveness(16,TYPE_BUG,TYPE_NONE)==MOVE_NEUTRAL); /* Gust Normal */
    assert(gen1_move_effectiveness(44,TYPE_GHOST,TYPE_NONE)==MOVE_IMMUNE); /* Bite Normal */
    assert(gen1_move_effectiveness(69,TYPE_GHOST,TYPE_NONE)==MOVE_NEUTRAL); /* Seismic Toss bug */
    assert(gen1_move_effectiveness(101,TYPE_NORMAL,TYPE_NONE)==MOVE_NEUTRAL);
    assert(gen1_move_effectiveness(165,TYPE_GHOST,TYPE_NONE)==MOVE_IMMUNE); /* Struggle still Normal */
    assert(gen1_move_effectiveness(138,TYPE_WATER,TYPE_NONE)==MOVE_UNKNOWN);
    struct gen3_move_context c={.first=TYPE_GHOST,.known=true};
    assert(gen3_move_effectiveness(69,&c)==MOVE_IMMUNE);c.identified=true;assert(gen3_move_effectiveness(69,&c)==MOVE_NEUTRAL);
    c.first=TYPE_NORMAL;assert(gen3_move_effectiveness(101,&c)==MOVE_IMMUNE);
    c.first=TYPE_FLYING;c.ability=26;assert(gen3_move_effectiveness(89,&c)==MOVE_IMMUNE);
    c.first=TYPE_WATER;c.ability=10;assert(gen3_move_effectiveness(85,&c)==MOVE_IMMUNE);
    c.ability=11;assert(gen3_move_effectiveness(57,&c)==MOVE_IMMUNE);
    c.first=TYPE_GRASS;c.ability=18;assert(gen3_move_effectiveness(53,&c)==MOVE_IMMUNE);c.frozen=true;assert(gen3_move_effectiveness(53,&c)==MOVE_SUPER);
    c.first=TYPE_BUG;c.second=TYPE_GHOST;c.ability=25;assert(gen3_move_effectiveness(53,&c)==MOVE_SUPER);assert(gen3_move_effectiveness(85,&c)==MOVE_IMMUNE);
    assert(gen3_move_effectiveness(165,&c)==MOVE_NEUTRAL);c.ability=43;assert(gen3_move_effectiveness(45,&c)==MOVE_IMMUNE);assert(gen3_move_effectiveness(304,&c)==MOVE_IMMUNE);
    c.first=TYPE_NORMAL;c.second=TYPE_NONE;c.ability=5;assert(gen3_move_effectiveness(329,&c)==MOVE_IMMUNE);
    c.ability=0;assert(gen3_move_effectiveness(311,&c)==MOVE_UNKNOWN);assert(gen3_move_effectiveness(267,&c)==MOVE_UNKNOWN);assert(gen3_move_effectiveness(248,&c)==MOVE_UNKNOWN);assert(gen3_move_effectiveness(353,&c)==MOVE_UNKNOWN);
    c.known=false;assert(gen3_move_effectiveness(1,&c)==MOVE_UNKNOWN);c.known=true;c.ambiguous=true;assert(gen3_move_effectiveness(1,&c)==MOVE_UNKNOWN);
    c.ambiguous=false;c.first=TYPE_ROCK;memset(c.ivs,0,6);assert(gen3_move_effectiveness(237,&c)==MOVE_SUPER); /* all zero -> Fighting */
    c.ivs[0]=32;assert(gen3_move_effectiveness(237,&c)==MOVE_UNKNOWN);
    assert(gen3_move_effectiveness(355,&c)==MOVE_UNKNOWN);assert(gen1_move_effectiveness(166,TYPE_NORMAL,TYPE_NONE)==MOVE_UNKNOWN);
}
int main(void)
{
    rules();kanto_tests();emerald_tests();
    puts("Gen 1/3 battle profiles: menu/transition/identity/target guards, move IDs, actual abilities, original type bugs and exhaustive chart parity passed");
    return 0;
}

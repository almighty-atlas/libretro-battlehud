#ifdef NDEBUG
#undef NDEBUG
#endif
#include "catch_decoder.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t ram[0x2000],rom[0x200000];static size_t missing;
static bool readmem(void *c,size_t a,void *out,size_t n)
{
    (void)c;if(missing && a<=missing && missing-a<n)return false;
    if(a>=0xc000 && a<0xe000 && n<=0xe000-a){memcpy(out,ram+a-0xc000,n);return true;}
    if(a>=0x10000000 && a<0x10200000 && n<=0x10200000-a){memcpy(out,rom+a-0x10000000,n);return true;}
    return false;
}
static void put(size_t a,unsigned v){assert(a>=0xc000 && a<0xe000);ram[a-0xc000]=(uint8_t)v;}
static void be(size_t a,unsigned v){put(a,v>>8);put(a+1,v);}
static struct battle_state state;
static const struct game_profile *p;
static struct catch_hint get(void){return catch_decode(p,readmem,NULL,&state);}
static void fixture(unsigned ball,unsigned slot)
{
    memset(ram,0,sizeof(ram));memset(rom,0,sizeof(rom));memset(&state,0,sizeof(state));missing=0;
    state.generation=2;state.status=BATTLE_ACTIVE;state.mode=1;state.species=161;state.type1=TYPE_NORMAL;
    const struct catch_profile *f=&p->catch_profile;
    put(f->pocket,1);put(p->stats_state,4);put(p->menu_data_pointer,f->menu_pointer&255);put(p->menu_data_pointer+1,f->menu_pointer>>8);
    put(p->menu_data_bank,4);const uint8_t borders[]={1,7,13,19},geom[]={5,8,2,0,0xd7,0xd8};
    memcpy(ram+f->borders-0xc000,borders,4);memcpy(ram+f->geometry-0xc000,geom,6);
    put(f->cursor,slot+1);put(f->balls,5);for(unsigned i=0;i<5;i++){put(f->balls+1+2*i,i==slot?ball:5);put(f->balls+2+2*i,10);}
    put(f->current_item,ball);put(f->selection,ball);for(unsigned i=0;i<20;i++)put(f->tilemap+i,0x28+i);
    put(f->tilemap+(slot+1)*40+7,0xed);put(f->original_enemy,161);put(f->catch_rate,255);put(p->party_count,1);
    put(f->player_level,50);put(p->enemy_level,10);be(p->enemy_hp,20);be(p->enemy_max_hp,20);
    put(f->original_player,155);uint8_t *m=ram+p->party_base-0xc000;m[0]=155;m[31]=50;m[21]=0xff;m[22]=0xff;
}
static void formulas(void)
{
    uint16_t chance;
    assert(crystal_catch_chance(255,20,20,0,false,&chance) && chance==3359); /* (85+1)/256 */
    assert(crystal_catch_chance(255,1,20,0,false,&chance) && chance==9648);
    assert(crystal_catch_chance(255,20,20,1,false,&chance) && chance==3750);
    for(unsigned status=8;status<=64;status*=2){assert(crystal_catch_chance(255,20,20,(uint8_t)status,false,&chance));assert(chance==(status==32?3750:3359));}
    assert(crystal_catch_chance(255,1,20,1,true,&chance) && chance==10000);
    assert(crystal_catch_chance(45,20,20,1,true,&chance) && chance==1796); /* Level skips HP and status. */
    assert(crystal_catch_chance(0,20,20,0,false,&chance) && chance==78);
    assert(!crystal_catch_chance(255,0,20,0,false,&chance));assert(!crystal_catch_chance(255,21,20,0,false,&chance));
    assert(!crystal_catch_chance(255,1,1000,0,false,&chance));assert(!crystal_catch_chance(255,1,20,128,false,&chance));
    assert(!crystal_catch_chance(255,1,20,9,false,&chance));assert(!crystal_catch_chance(255,1,20,24,false,&chance));
    /* Actual byte arithmetic around overflow: 342/683/684 denominator wraps to zero. */
    for(unsigned max=1;max<=999;max++)for(unsigned hp=1;hp<=max;hp++) {
        bool known=crystal_catch_chance(127,(uint16_t)hp,(uint16_t)max,0,false,&chance);
        unsigned denom=3*max;if(denom>255)denom>>=2;
        assert(known==((denom&255)!=0));if(known)assert(chance>=78 && chance<=10000);
    }
}
static void menus(void)
{
    const unsigned balls[]={1,2,4,5,0x9d,0x9f,0xa0,0xa1,0xa4,0xa5,0xa6};
    for(unsigned b=0;b<sizeof(balls)/sizeof(*balls);b++)for(unsigned slot=0;slot<5;slot++){
        fixture(balls[b],slot);struct catch_hint h=get();assert(h.visible && h.ball==balls[b]);
        if(balls[b]!=0x9d)assert(h.known);else assert(!h.known); /* No invented weight from an empty ROM. */
        if(balls[b]==1)assert(h.master && h.permyriad==10000);
    }
    const struct catch_profile *f=&p->catch_profile;
    const size_t menu_reads[]={f->switch_item,f->pocket,p->stats_state,p->menu_data_pointer,p->menu_data_bank,f->borders,f->geometry,f->cursor,f->scroll,f->balls,
        f->balls+1,f->balls+2,f->current_item,f->selection,f->tilemap,f->tilemap+47};
    for(unsigned i=0;i<sizeof(menu_reads)/sizeof(*menu_reads);i++){
        fixture(5,0);missing=menu_reads[i];assert(!get().visible);
        fixture(5,0);put(menu_reads[i],255);assert(!get().visible);
    }
    fixture(5,0);put(f->scroll,1);put(f->balls+3,2);put(f->current_item,2);put(f->selection,2);assert(get().known && get().ball==2);
    fixture(5,0);put(f->scroll,5);assert(!get().visible); /* CANCEL */
    fixture(5,0);state.mode=2;assert(!get().visible);fixture(5,0);state.status=BATTLE_TRANSITION;assert(!get().visible);
    fixture(5,0);state.main_menu=true;assert(!get().visible);fixture(5,0);state.fight_menu=true;assert(!get().visible);
    fixture(5,0);state.training.visible=true;assert(!get().visible);
    const size_t inputs[]={f->battle_type,f->original_enemy,f->catch_rate,f->enemy_status,p->enemy_hp,p->enemy_max_hp,p->party_count};
    for(unsigned i=0;i<sizeof(inputs)/sizeof(*inputs);i++){fixture(5,0);missing=inputs[i];assert(get().visible && !get().known);}
    fixture(1,0);put(p->party_count,6);assert(!get().known); /* Master cannot certify unknown full-box capacity. */
    fixture(5,0);put(f->battle_type,3);assert(!get().known);
    fixture(5,0);be(p->enemy_max_hp,342);assert(!get().known);
    fixture(5,0);put(f->catch_rate,45);uint16_t poke=get().permyriad;
    fixture(0xa5,0);put(f->catch_rate,45);assert(get().permyriad==poke); /* Moon bug */
    fixture(0xa0,0);put(f->catch_rate,45);assert(get().permyriad==poke);put(f->battle_type,4);assert(get().permyriad>poke);
    fixture(0x9f,0);put(f->catch_rate,45);put(f->player_level,40);assert(get().permyriad==7070); /* 4x, not 8x at equality */
    put(f->player_level,44);assert(get().permyriad==10000);
    fixture(0xa1,0);put(f->catch_rate,45);state.species=81;put(f->original_enemy,81);assert(get().permyriad>poke);
    state.species=133;put(f->original_enemy,133);assert(get().permyriad==poke); /* Fast ignores most flee species. */
    /* Heavy reads the ROM's species-bank selection, including the three wrong-bank boundaries. */
    fixture(0x9d,0);put(f->catch_rate,45);unsigned index=(161>>6)&3;rom[f->dex_banks-0x10000000+index]=0x20;
    size_t ptr=f->dex_pointers-0x10000000+160*2;rom[ptr]=0;rom[ptr+1]=0x40;
    rom[0x80000]=0x50;rom[0x80003]=100;rom[0x80004]=0;assert(get().known && get().permyriad<poke);
    missing=f->dex_banks+index;assert(!get().known);
    /* Love uses the original same-gender bug, checking party DVs and ROM gender ratio. */
    fixture(0xa6,0);put(f->catch_rate,45);put(f->original_player,161);put(p->party_base,161);
    size_t base=p->species_info-0x10000000+160*32;rom[base]=161;rom[base+13]=127;
    put(f->enemy_dvs,0xf0);put(f->enemy_dvs+1,0xf0);assert(get().known && get().permyriad>poke);
    put(f->enemy_dvs,0);put(f->enemy_dvs+1,0);assert(get().known && get().permyriad==poke);
    rom[base+13]=255;assert(get().known && get().permyriad==poke);
    missing=p->species_info+160*32;assert(!get().known);
    fixture(5,0);put(p->battle_mode,1);put(p->enemy_species,161);put(p->enemy_type1,0);put(p->enemy_type2,0);
    struct battle_state integrated=battle_decode(p,readmem,NULL);
    assert(integrated.catch_hint.visible && integrated.catch_hint.known && integrated.catch_hint.permyriad==3359);
    put(p->battle_mode,2);assert(!battle_decode(p,readmem,NULL).catch_hint.visible);
    assert(!catch_decode(NULL,readmem,NULL,&state).visible);
    struct game_profile other=*p;other.generation=3;assert(!catch_decode(&other,readmem,NULL,&state).visible);
}
int main(void)
{p=game_profile_find("f2f52230b536214ef7c9924f483392993e226cfb");assert(p);formulas();menus();puts("Crystal catch: all HP pairs, original inclusive roll/status/overflow/ball bugs, selection/scroll/pocket/transition/capacity/missing-data guards passed");}

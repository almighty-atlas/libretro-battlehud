#include "multigen_battle.h"
#include "multigen_effectiveness.h"
#include <string.h>
#include "gen1_species_data.h"
static unsigned le16(const uint8_t *p){return p[0]|(unsigned)p[1]<<8;}
static uint32_t le32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static unsigned be16(const uint8_t *p){return (unsigned)p[0]*256+p[1];}
static bool u8(battle_memory_read read,void *c,size_t a,uint8_t *v){return read(c,a,v,1);}
static bool u32(battle_memory_read read,void *c,size_t a,uint32_t *v)
{uint8_t b[4];if(!read(c,a,b,4))return false;*v=le32(b);return true;}
static bool labels(const struct menu_label *l,battle_memory_read read,void *c)
{
    for(unsigned i=0;i<4;i++){uint8_t b[8];if(!l[i].size || l[i].size>8 ||
        !read(c,l[i].address,b,l[i].size) || memcmp(b,l[i].tiles,l[i].size))return false;}
    return true;
}
static bool gen1_type(uint8_t raw,enum pokemon_type *type)
{return raw!=9 && raw!=27 && gen2_type_decode(raw,type);}
bool gen3_type_decode(uint8_t raw,enum pokemon_type *type)
{
    static const enum pokemon_type types[]={TYPE_NORMAL,TYPE_FIGHTING,TYPE_FLYING,TYPE_POISON,
        TYPE_GROUND,TYPE_ROCK,TYPE_BUG,TYPE_GHOST,TYPE_STEEL,TYPE_NONE,TYPE_FIRE,TYPE_WATER,
        TYPE_GRASS,TYPE_ELECTRIC,TYPE_PSYCHIC,TYPE_ICE,TYPE_DRAGON,TYPE_DARK};
    if(!type || raw>=18 || types[raw]==TYPE_NONE)return false;
    *type=types[raw];return true;
}
static struct battle_state kanto(const struct game_profile *p,battle_memory_read read,void *c)
{
    struct battle_state s={0};s.generation=1;s.status=BATTLE_UNAVAILABLE;
    uint8_t mode,kind,escaped,start,identity,mon[17],types[2];
    if(!u8(read,c,p->battle_mode,&mode))return s;
    if(!mode){s.status=BATTLE_OUTSIDE;return s;}
    if(mode!=1 && mode!=2){s.status=BATTLE_INVALID;return s;}
    if(!u8(read,c,p->battle_type,&kind) || !u8(read,c,p->escaped,&escaped) ||
       !u8(read,c,p->battle_starting,&start) || !u8(read,c,p->enemy_identity,&identity) ||
       !read(c,p->enemy_species,mon,sizeof(mon)) ||
       !u8(read,c,p->enemy_type1,&types[0]) || !u8(read,c,p->enemy_type2,&types[1]))return s;
    if(escaped){s.status=BATTLE_OUTSIDE;return s;}
    s.mode=mode;
    if(kind || start || !be16(mon+1) || identity!=mon[0]){s.status=BATTLE_TRANSITION;return s;}
    unsigned raw=mon[0];enum pokemon_type t1,t2;
    if(!raw || raw>190 || !kanto_dex[raw] || !mon[14] || mon[14]>100 ||
       !be16(mon+15) || be16(mon+15)>999 || be16(mon+1)>be16(mon+15) ||
       !gen1_type(types[0],&t1) || !gen1_type(types[1],&t2)){s.status=BATTLE_INVALID;return s;}
    s.status=BATTLE_ACTIVE;s.species=kanto_dex[raw];s.type1=t1;s.type2=t1==t2?TYPE_NONE:t2;
    s.raw_type1=types[0];s.raw_type2=types[1];
    uint8_t geometry[6],box,move_kind,rows;
    if(!read(c,p->move_geometry,geometry,6) || !u8(read,c,p->menu_data_pointer,&box) ||
       !u8(read,c,p->move_menu_type,&move_kind) || !u8(read,c,p->move_rows,&rows))return s;
    s.main_menu=box==p->main_menu_pointer && geometry[0]==14 &&
        (geometry[1]==9 || geometry[1]==15) && geometry[4]==1 && labels(p->main_menu_labels,read,c);
    s.fight_menu=move_kind==0 && geometry[0]==12 && geometry[1]==5 && rows<4 &&
        geometry[4]==rows+2 && labels(p->move_menu_labels,read,c);
    if(s.fight_menu){
        uint8_t moves[4],pp[4],disabled;
        if(!read(c,p->player_moves,moves,4) || !read(c,p->player_pp,pp,4) || !u8(read,c,p->player_disable,&disabled))return s;
        unsigned count=0;while(count<4 && moves[count] && moves[count]<=165)count++;
        bool valid=count==(unsigned)rows+1;for(unsigned i=count;i<4;i++)if(moves[i])valid=false;
        if(valid)for(unsigned i=0;i<count;i++){
            s.moves[i]=moves[i];s.effectiveness[i]=!(pp[i]&63) || ((disabled&15) && (disabled>>4)==i+1)?
                MOVE_UNUSABLE:gen1_move_effectiveness(moves[i],t1,s.type2);
        }
    }
    return s;
}
static bool mon_valid(const uint8_t *m)
{
    unsigned species=le16(m),hp=le16(m+40),max=le16(m+44);
    enum pokemon_type a,b;
    return species && species<=411 && (species<252 || species>276) &&
        m[42] && m[42]<=100 && hp && max && max<=999 && hp<=max && m[32]<78 &&
        !(m[23]&64) && gen3_type_decode(m[33],&a) && gen3_type_decode(m[34],&b);
}
static struct battle_state emerald(const struct game_profile *p,battle_memory_read read,void *c)
{
    struct battle_state s={0};s.generation=3;s.status=BATTLE_UNAVAILABLE;
    const struct gba_battle_profile *b=&p->gba_battle;
    uint32_t callback,flags,exec;uint8_t count,positions[4],absent,outcome,fade,scroll[4];
    if(!u32(read,c,p->main_callback,&callback))return s;
    if(callback!=b->callback){s.status=BATTLE_OUTSIDE;return s;}
    if(!u32(read,c,b->flags,&flags) || !u8(read,c,b->count,&count) || !u8(read,c,b->outcome,&outcome) ||
       !u8(read,c,p->palette_fade+7,&fade) || !read(c,b->bg_scroll,scroll,4) ||
       !read(c,b->positions,positions,4) || !u8(read,c,b->absent,&absent) || !u32(read,c,b->exec,&exec))return s;
    if(outcome){s.status=BATTLE_OUTSIDE;return s;}
    /* Ordinary wild/trainer singles/doubles only; no link, Safari, tutorial,
     * Frontier, recorded, or alternate-controller menus. */
    const uint32_t allowed=1u|4u|8u|16u|0x400u|0x1000u|0x2000u|0x4000u|0x8000u|0x400000u|0x70000000u;
    if((flags&~allowed) || (count!=2 && count!=4) || ((flags&1)!=0)!=(count==4)){
        s.status=BATTLE_INVALID;return s;
    }
    s.mode=(flags&8)?2:1;
    if(fade&128){s.status=BATTLE_TRANSITION;return s;}
    unsigned player=4,foe=4,foes=0,eligible=0,seen=0;bool fight=false;
    uint8_t mons[4][88];
    for(unsigned i=0;i<count;i++){
        if(positions[i]>=count || (seen&(1u<<positions[i]))){s.status=BATTLE_INVALID;return s;}seen|=1u<<positions[i];
        if(absent&(1u<<i))continue;
        if(!read(c,b->mons+i*88,mons[i],88))return s;
        if(!mon_valid(mons[i])){s.status=BATTLE_TRANSITION;return s;}
        if(positions[i]&1){foe=i;foes++;continue;}
        uint32_t function;uint8_t command;
        if(!u32(read,c,b->controllers+i*4,&function) || !u8(read,c,b->buffer+i*512,&command))return s;
        if(!(exec&(1u<<i)))continue;
        if((function==b->action_input && command==18 && le16(scroll)==0 && le16(scroll+2)==160) ||
           (function==b->move_input && command==20 && le16(scroll)==0 && le16(scroll+2)==320)){
            eligible++;player=i;fight=function==b->move_input;
        }
    }
    if(!foes || eligible!=1){s.status=BATTLE_TRANSITION;return s;}
    s.status=BATTLE_ACTIVE;s.ambiguous_target=foes!=1;
    /* Types are retained in the model, but ambiguous opponent badges are hidden. */
    uint8_t *enemy=mons[foe],*own=mons[player];s.species=(uint16_t)le16(enemy);
    gen3_type_decode(enemy[33],&s.type1);gen3_type_decode(enemy[34],&s.type2);
    if(s.type1==s.type2)s.type2=TYPE_NONE;
    s.raw_type1=enemy[33];s.raw_type2=enemy[34];
    s.fight_menu=fight;s.main_menu=!fight;
    if(fight){
        uint8_t selection[24],disable[28];
        if(!read(c,b->buffer+player*512,selection,sizeof(selection)) || !read(c,b->disable+player*28,disable,sizeof(disable)))return s;
        if(le16(selection+20)!=le16(own) || selection[22]!=own[33] || selection[23]!=own[34])return s;
        struct gen3_move_context m={.first=s.type1,.second=s.type2,.ability=enemy[32],.known=true,
            .identified=(le32(enemy+80)&(1u<<29))!=0,.frozen=(le32(enemy+76)&32)!=0,.ambiguous=s.ambiguous_target};
        uint32_t iv=le32(own+20);const unsigned order[]={0,1,2,4,5,3};
        for(unsigned i=0;i<6;i++)m.ivs[i]=(uint8_t)((iv>>(order[i]*5))&31);
        bool valid=true;unsigned n=0;
        for(unsigned i=0;i<4;i++){
            unsigned move=le16(selection+4+i*2);
            if(move!=le16(own+12+i*2) || move>354 || selection[12+i]!=own[36+i])valid=false;
            if(move){if(i!=n)valid=false;n++;}
        }
        if(!n)valid=false;
        if(valid)for(unsigned i=0;i<n;i++){
            s.moves[i]=(uint16_t)le16(selection+4+i*2);
            s.effectiveness[i]=!selection[12+i] || ((disable[11]&15) && le16(disable+4)==s.moves[i])?
                MOVE_UNUSABLE:gen3_move_effectiveness(s.moves[i],&m);
        }
    }
    return s;
}
struct battle_state multigen_battle_decode(const struct game_profile *p,battle_memory_read read,void *ctx)
{
    if(!read || !p){struct battle_state s={.status=BATTLE_UNAVAILABLE};return s;}
    return p->generation==1?kanto(p,read,ctx):emerald(p,read,ctx);
}

#include "catch_decoder.h"
#include <string.h>
static bool byte(battle_memory_read r,void *c,size_t a,uint8_t *b){return r(c,a,b,1);}
static bool word(battle_memory_read r,void *c,size_t a,uint16_t *v)
{uint8_t b[2];if(!r(c,a,b,2))return false;*v=(uint16_t)(b[0]*256u+b[1]);return true;}
static unsigned saturate(unsigned n){return n>255?255:n;}
static bool heavy(const struct game_profile *p,battle_memory_read r,void *c,unsigned species,unsigned *rate)
{
    uint8_t ptr[2],bank,ch,weight[2];
    if(!r(c,p->catch_profile.dex_pointers+(species-1)*2,ptr,2))return false;
    /* RLCA twice, not a left shift: bits 6/7 become the bank-table index. */
    unsigned index=((species>>6)|((species<<2)&255))&3;
    if(!byte(r,c,p->catch_profile.dex_banks+index,&bank))return false;
    unsigned cpu=ptr[0]|(unsigned)ptr[1]<<8;
    if(cpu<0x4000 || cpu>0x7ffb || !bank)return false;
    size_t end_addr=0x10000000+(size_t)(bank+1)*0x4000;
    size_t addr=0x10000000+(size_t)bank*0x4000+cpu-0x4000;bool end=false;
    for(unsigned i=0;i<32;i++){if(addr>=end_addr || !byte(r,c,addr++,&ch))return false;if(ch==0x50){end=true;break;}}
    if(!end || addr+4>end_addr || !r(c,addr+2,weight,2))return false;
    unsigned v=(weight[0]|(unsigned)weight[1]<<8)/2;
    unsigned high=(v-v/16-v/32)>>8;
    if(high<4)*rate=*rate<20?1:*rate-20;
    else *rate=saturate(*rate+(high<8?0:high<12?20:high<16?30:40));
    return true;
}
static bool gender(const struct game_profile *p,battle_memory_read r,void *c,unsigned species,
                   unsigned attack,unsigned speed,int *out)
{
    uint8_t record[14];
    if(!r(c,p->species_info+(species-1)*32,record,14) || record[0]!=species)return false;
    unsigned ratio=record[13],dv=attack*16+speed;
    *out=ratio==255?-1:ratio==0?0:ratio==254?1:dv<=ratio?1:0;return true;
}
struct catch_hint catch_decode(const struct game_profile *p,battle_memory_read r,void *c,const struct battle_state *s)
{
    struct catch_hint h={0};
    if(!p || !r || !s || p->generation!=2 || !p->catch_profile.menu_pointer ||
       !s->species || s->species>251 || s->status!=BATTLE_ACTIVE || s->mode!=1 || s->main_menu || s->fight_menu || s->training.visible)return h;
    const struct catch_profile *f=&p->catch_profile;
    uint8_t pocket,state,bank,borders[4],geometry[6],cursor,scroll,count,item,cur,selection,qty,arrow,switch_item,top[20];
    uint8_t pointer_bytes[2];
    /* Validate the live scrolling list, selected record, matching item/description
     * selection and rendered pack header/cursor. Do not reuse a latched thrown item. */
    if(!byte(r,c,f->switch_item,&switch_item) || switch_item || !byte(r,c,f->pocket,&pocket) || pocket!=1 || !byte(r,c,p->stats_state,&state) || state!=4 ||
       !r(c,p->menu_data_pointer,pointer_bytes,2) || (pointer_bytes[0]|(unsigned)pointer_bytes[1]<<8)!=f->menu_pointer ||
       !byte(r,c,p->menu_data_bank,&bank) || bank!=4 || !r(c,f->borders,borders,4) ||
       memcmp(borders,(uint8_t[]){1,7,13,19},4) || !r(c,f->geometry,geometry,6) ||
       memcmp(geometry,(uint8_t[]){5,8,2,0,0xd7,0xd8},6) ||
       !byte(r,c,f->cursor,&cursor) || !cursor || cursor>5 || !byte(r,c,f->scroll,&scroll) ||
       !byte(r,c,f->balls,&count) || !count || count>12 || scroll+cursor-1>=count ||
       !byte(r,c,f->balls+1+(scroll+cursor-1)*2,&item) || !crystal_ball_name(item) ||
       !byte(r,c,f->balls+2+(scroll+cursor-1)*2,&qty) || !qty || qty>99 ||
       !byte(r,c,f->current_item,&cur) || cur!=item || !byte(r,c,f->selection,&selection) || selection!=item ||
       !r(c,f->tilemap,top,20) || !byte(r,c,f->tilemap+cursor*40+7,&arrow) || arrow!=0xed)return h;
    for(unsigned i=0;i<20;i++)if(top[i]!=0x28+i)return h;
    h.visible=true;h.ball=item;
    uint8_t kind,rate,status,enemy,count_party;
    uint16_t hp,maxhp;
    if(!byte(r,c,f->battle_type,&kind) || kind==2 || kind==3 || kind==6 || kind>12 ||
       !byte(r,c,f->original_enemy,&enemy) || enemy!=s->species ||
       !byte(r,c,f->catch_rate,&rate) || !rate || !byte(r,c,f->enemy_status,&status) ||
       !word(r,c,p->enemy_hp,&hp) || !word(r,c,p->enemy_max_hp,&maxhp) || !hp || hp>maxhp || maxhp>999 ||
       !byte(r,c,p->party_count,&count_party) || !count_party || count_party>=6)return h;
    /* A six-member team requires a banked SRAM box-capacity read. Until mapped
     * unambiguously, no throw/capture certainty is invented (including Master). */
    unsigned major=status&0x78;
    if(status&0x80 || (major && (major&(major-1))) || (major && (status&7)))return h;
    unsigned adjusted=rate;
    if(item==1){h.known=h.master=true;h.permyriad=10000;return h;}
    if(item==2)adjusted=saturate(rate*2u);
    if(item==4)adjusted=saturate(rate+rate/2u);
    if(item==0xa0 && kind==4)adjusted=saturate(rate*3u);
    if(item==0xa1 && (enemy==81 || enemy==88 || enemy==114))adjusted=saturate(rate*4u);
    /* Moon's Burn Heal evolution test never boosts any species in this pinned ROM. */
    if(item==0x9d && !heavy(p,r,c,enemy,&adjusted))return h;
    if(item==0x9f){
        uint8_t player,foe;if(!byte(r,c,f->player_level,&player) || !byte(r,c,p->enemy_level,&foe) ||
           !player || player>100 || !foe || foe>100)return h;
        adjusted=saturate(rate*(player/4>foe?8u:player/2>foe?4u:player>foe?2u:1u));
    }
    if(item==0xa6){
        uint8_t player,slot,dvs[2];
        if(!byte(r,c,f->original_player,&player) || !player || player>251)return h;
        if(player==enemy){
            if(!byte(r,c,f->player_slot,&slot) || slot>=count_party || !r(c,f->enemy_dvs,dvs,2))return h;
            struct training_stats mon=training_party_mon(p,r,c,slot);int own,foe;
            if(!mon.visible || mon.species!=player || !gender(p,r,c,enemy,mon.dv[1],mon.dv[5],&own) ||
               !gender(p,r,c,enemy,dvs[0]>>4,dvs[1]>>4,&foe))return h;
            if(own>=0 && own==foe)adjusted=saturate(rate*8u); /* Original same-gender bug. */
        }
    }
    h.known=crystal_catch_chance((uint8_t)adjusted,hp,maxhp,status,item==0x9f,&h.permyriad);
    return h;
}

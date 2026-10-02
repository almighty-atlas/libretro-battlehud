#include "training_stats.h"
#include "party_details.h"
#include <string.h>
static struct training_stats crystal_decode(const struct game_profile *p,
                                           training_memory_read read, void *context)
{
    struct training_stats s={0};
    uint8_t flags,state,source,index,count,species,marker[2],temp[48],party[48];
    if(!p || !read || !read(context,p->stats_flags,&flags,1) || (flags&3)!=3 ||
       !read(context,p->stats_state,&state,1) || state!=6 ||
       !read(context,p->mon_source,&source,1) || source!=0 ||
       !read(context,p->party_index,&index,1) ||
       !read(context,p->party_count,&count,1) || !count || count>6 || index>=count ||
       !read(context,p->party_species,&species,1) || !species || species>251 ||
       !read(context,p->stats_page_marker,marker,2) || marker[0]!=0x3a || marker[1]!=0x3b)
        return s;
    for(unsigned i=0;i<5;i++) {
        const struct menu_label *label=&p->stats_labels[i]; uint8_t tiles[8];
        if(!label->size || label->size>sizeof(tiles) ||
           !read(context,label->address,tiles,label->size) || memcmp(tiles,label->tiles,label->size))
            return s;
    }
    if(!read(context,p->temp_mon,temp,sizeof(temp)) ||
       !read(context,(size_t)p->party_base+index*sizeof(party),party,sizeof(party)) ||
       memcmp(temp,party,sizeof(temp)) || temp[0]!=species || !temp[31] || temp[31]>100)
        return s;
    uint8_t attack=temp[21]>>4,defense=temp[21]&15,speed=temp[22]>>4,special=temp[22]&15;
    s.dv[0]=(uint8_t)((attack&1)*8+(defense&1)*4+(speed&1)*2+(special&1));
    s.dv[1]=attack; s.dv[2]=defense; s.dv[3]=special; s.dv[4]=special; s.dv[5]=speed;
    const unsigned offsets[6]={11,13,15,19,19,17};
    for(unsigned i=0;i<6;i++) s.ev[i]=(uint16_t)((unsigned)temp[offsets[i]]*256+temp[offsets[i]+1]);
    s.slot=index; s.generation=2; s.species=species; s.visible=true;
    return s;
}
/* All multibyte decoding uses byte operations; host endian/alignment is irrelevant. */
static uint16_t le16(const uint8_t *p) { return (uint16_t)(p[0]|(unsigned)p[1]<<8); }
static uint32_t le32(const uint8_t *p) { return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static bool read32(training_memory_read read,void *ctx,size_t addr,uint32_t *out)
{
    uint8_t b[4]; if(!read(ctx,addr,b,4)) return false; *out=le32(b); return true;
}
static struct training_stats kanto_decode(const struct game_profile *p,training_memory_read read,void *ctx)
{
    struct training_stats s={0}; uint8_t source,index,count,temp[44],party[44];
    if(!read(ctx,p->mon_source,&source,1) || source!=0 ||
       !read(ctx,p->party_index,&index,1) || !read(ctx,p->party_count,&count,1) ||
       !count || count>6 || index>=count) return s;
    for(unsigned i=0;i<5;i++) {
        uint8_t b[8]; const struct menu_label *l=&p->stats_labels[i];
        if(!l->size || l->size>8 || !read(ctx,l->address,b,l->size) || memcmp(b,l->tiles,l->size)) return s;
    }
    if(!read(ctx,p->temp_mon,temp,44) || !read(ctx,p->party_base+index*44,party,44) ||
       memcmp(temp,party,44) || !temp[0] || temp[0]>190 || !temp[33] || temp[33]>100) return s;
    unsigned a=temp[27]>>4,d=temp[27]&15,v=temp[28]>>4,c=temp[28]&15;
    s.dv[0]=(uint8_t)((a&1)*8+(d&1)*4+(v&1)*2+(c&1));
    s.dv[1]=(uint8_t)a;s.dv[2]=(uint8_t)d;s.dv[3]=s.dv[4]=(uint8_t)c;s.dv[5]=(uint8_t)v;
    const unsigned offsets[]={17,19,21,25,25,23};
    for(unsigned i=0;i<6;i++) s.ev[i]=(uint16_t)((unsigned)temp[offsets[i]]*256+temp[offsets[i]+1]);
    s.visible=true;s.slot=index;s.species=temp[0];s.generation=1;return s;
}
static struct training_stats emerald_decode(const struct game_profile *p,training_memory_read read,void *ctx)
{
    struct training_stats s={0}; uint32_t ptr,callback,list;
    uint8_t fade,task[8],screen[5],count,temp[100],party[100]; bool input=false;
    if(!read32(read,ctx,p->main_callback,&callback) || callback!=p->summary_callback ||
       !read(ctx,p->palette_fade+7,&fade,1) || (fade&0x80)) return s;
    for(unsigned i=0;i<16;i++) {
        if(!read(ctx,p->tasks+i*40,task,8)) return s;
        if(task[4] && le32(task)==p->input_task) input=true;
    }
    if(!input || !read32(read,ctx,p->summary_pointer,&ptr) || ptr<0x02000000 ||
       ptr>0x02040000-0x40c1 || (ptr&3) || !read32(read,ctx,ptr,&list) || list!=p->gba_party_base ||
       !read(ctx,ptr+0x40bc,screen,5) || screen[0]>1 || screen[1] || screen[4]!=1 ||
       !read(ctx,p->gba_party_count,&count,1) || !count || count>6 ||
       screen[2]>=count || screen[3]!=count-1 ||
       !read(ctx,ptr+12,temp,100) || !read(ctx,p->gba_party_base+screen[2]*100,party,100) ||
       memcmp(temp,party,100) || (temp[19]&7)!=2 || !temp[84] || temp[84]>100) return s;
    uint8_t clear[48]; uint32_t pid=le32(temp),key=pid^le32(temp+4);
    for(unsigned i=0;i<48;i++) clear[i]=temp[32+i]^(uint8_t)(key>>(8*(i%4)));
    unsigned checksum=0;for(unsigned i=0;i<48;i+=2) checksum+=le16(clear+i);
    if((uint16_t)checksum!=le16(temp+28)) return s;
    /* Logical substructures (Growth, EVs, Misc) -> physical 12-byte block indices.
     * Derived from GetSubstruct/SUBSTRUCT_CASE, not assumed alphabetical order. */
    static const uint8_t positions[24][3]={
        {0,2,3},{0,3,2},{0,1,3},{0,1,2},{0,3,1},{0,2,1},
        {1,2,3},{1,3,2},{2,1,3},{3,1,2},{2,3,1},{3,2,1},
        {1,0,3},{1,0,2},{2,0,3},{3,0,2},{2,0,1},{3,0,1},
        {1,3,0},{1,2,0},{2,3,0},{3,2,0},{2,1,0},{3,1,0}
    };
    const uint8_t *pos=positions[pid%24],*ev=clear+pos[1]*12;
    uint16_t species=le16(clear+pos[0]*12);uint32_t iv=le32(clear+pos[2]*12+4);
    if(!species || species>411 || (species>=252 && species<=276) || (iv&(1u<<30))) return s;
    const unsigned order[]={0,1,2,4,5,3};unsigned total=0;
    for(unsigned i=0;i<6;i++) {s.dv[i]=(uint8_t)((iv>>(order[i]*5))&31);s.ev[i]=ev[order[i]];total+=s.ev[i];}
    if(total>510) return (struct training_stats){0};
    s.visible=true;s.slot=screen[2];s.species=species;s.generation=3;
    s.nature_known=true;s.nature=(uint8_t)(pid%25);s.ability_slot=(uint8_t)(iv>>31);
    s.ability_known=gen3_ability_read(p,read,ctx,species,s.ability_slot,&s.ability,s.ability_name);
    return s;
}
struct training_stats training_stats_decode(const struct game_profile *p,training_memory_read read,void *ctx)
{
    if(!p || !read) return (struct training_stats){0};
    if(p->generation==1) return kanto_decode(p,read,ctx);
    if(p->generation==2) return crystal_decode(p,read,ctx);
    if(p->generation==3) return emerald_decode(p,read,ctx);
    return (struct training_stats){0};
}
bool training_stats_equal(const struct training_stats *a,const struct training_stats *b)
{
    return a->visible==b->visible && a->generation==b->generation && a->slot==b->slot && a->species==b->species &&
        a->nature_known==b->nature_known && a->ability_known==b->ability_known &&
        a->nature==b->nature && a->ability==b->ability && a->ability_slot==b->ability_slot &&
        !memcmp(a->ability_name,b->ability_name,sizeof(a->ability_name)) &&
        !memcmp(a->dv,b->dv,sizeof(a->dv)) && !memcmp(a->ev,b->ev,sizeof(a->ev));
}

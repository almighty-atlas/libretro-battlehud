#include "training_stats.h"
#include <string.h>
struct training_stats training_stats_decode(const struct game_profile *p,
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
    s.slot=index; s.species=species; s.visible=true;
    return s;
}
bool training_stats_equal(const struct training_stats *a,const struct training_stats *b)
{
    return a->visible==b->visible && a->slot==b->slot && a->species==b->species &&
        !memcmp(a->dv,b->dv,sizeof(a->dv)) && !memcmp(a->ev,b->ev,sizeof(a->ev));
}

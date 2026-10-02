#include "party_details.h"
#include <string.h>
static const char *const nature_names[]={
    "HARDY","LONELY","BRAVE","ADAMANT","NAUGHTY","BOLD","DOCILE","RELAXED",
    "IMPISH","LAX","TIMID","HASTY","SERIOUS","JOLLY","NAIVE","MODEST",
    "MILD","QUIET","BASHFUL","RASH","CALM","GENTLE","SASSY","CAREFUL","QUIRKY"
};
const char *gen3_nature_name(uint8_t nature)
{ return nature<25 ? nature_names[nature] : NULL; }
int gen3_nature_effect(uint8_t nature,unsigned stat)
{
    if(nature>=25 || stat==0 || stat>=6 || nature/5==nature%5) return 0;
    const unsigned displayed_to_game[]={0,0,1,3,4,2};
    unsigned game=displayed_to_game[stat];
    return nature/5==game ? 1 : nature%5==game ? -1 : 0;
}
bool gen3_ability_read(const struct game_profile *p,training_memory_read read,void *ctx,
                       uint16_t species,uint8_t slot,uint8_t *ability,char name[13])
{
    if(!p || p->generation!=3 || !p->species_info || !p->ability_names || !read ||
       !species || species>411 || (species>=252 && species<=276) || slot>1 || !ability || !name)
        return false;
    uint8_t id,encoded[13];char decoded[13]={0};
    /* SpeciesInfo stride 28, abilities at +22/+23. Slot is the stored bit, not PID parity. */
    if(!read(ctx,p->species_info+(size_t)species*28+22+slot,&id,1) || id>=78) return false;
    if(id==0) { *ability=0;memset(name,0,13);memcpy(name,"NONE",4);return true; }
    if(!read(ctx,p->ability_names+(size_t)id*13,encoded,13)) return false;
    bool letter=false;unsigned i;
    for(i=0;i<13 && encoded[i]!=0xff;i++) {
        if(i==12) return false; /* Name must have an EOS within its 13-byte entry. */
        if(encoded[i]>=0xbb && encoded[i]<=0xd4) decoded[i]=(char)('A'+encoded[i]-0xbb);
        else if(encoded[i]>=0xd5 && encoded[i]<=0xee) decoded[i]=(char)('A'+encoded[i]-0xd5);
        else if(encoded[i]==0) decoded[i]=' ';
        else return false;
        if(decoded[i]!=' ') letter=true;
    }
    if(!letter || i==13) return false;
    *ability=id;memcpy(name,decoded,13);return true;
}

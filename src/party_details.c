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

bool crystal_shiny(const uint8_t dv[6])
{ return dv && (dv[1]&2) && dv[2]==10 && dv[3]==10 && dv[5]==10; }
bool emerald_shiny(uint32_t pid,uint32_t ot)
{ return ((pid&65535)^(pid>>16)^(ot&65535)^(ot>>16))<8; }
unsigned party_pokerus_state(uint8_t p)
{ return !p?0:(p&15)?1:2; }
bool party_gender_read(const struct game_profile *p,training_memory_read read,void *ctx,
                       uint16_t species,uint32_t determinant,uint8_t *gender)
{
    if(!p || !read || !gender || !p->species_info || !species) return false;
    uint8_t ratio;
    if(p->generation==2) {
        uint8_t record[14];
        if(species>251 || !read(ctx,p->species_info+(species-1)*32,record,14) || record[0]!=species)return false;
        ratio=record[13];
    } else if(p->generation==3) {
        if(species>411 || (species>=252 && species<=276) ||
           !read(ctx,p->species_info+(size_t)species*28+16,&ratio,1))return false;
    } else return false;
    *gender=ratio==255?2:ratio==0?0:ratio==254?1:
        p->generation==2 ? (determinant&255)<=ratio : (determinant&255)<ratio;
    return true;
}

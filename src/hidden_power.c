#include "hidden_power.h"
/* Shared canonical order: neither Normal nor the unused Bird/Mystery type occurs.
 * Sources and exact generation-specific formulas: docs/hidden-power.md. */
static const enum pokemon_type types[16]={
    TYPE_FIGHTING,TYPE_FLYING,TYPE_POISON,TYPE_GROUND,TYPE_ROCK,TYPE_BUG,
    TYPE_GHOST,TYPE_STEEL,TYPE_FIRE,TYPE_WATER,TYPE_GRASS,TYPE_ELECTRIC,
    TYPE_PSYCHIC,TYPE_ICE,TYPE_DRAGON,TYPE_DARK
};
enum pokemon_type hidden_power_gen2_type(uint8_t attack,uint8_t defense)
{
    if(attack>15 || defense>15) return TYPE_NONE;
    return types[(attack&3)*4+(defense&3)];
}
bool hidden_power_calculate(unsigned generation,const uint8_t stats[6],struct hidden_power *out)
{
    if(!out) return false;
    *out=(struct hidden_power){0};
    if(!stats || (generation!=2 && generation!=3)) return false;
    unsigned max=generation==2 ? 15 : 31;
    for(unsigned i=0;i<6;i++) if(stats[i]>max) return false;
    if(generation==2) {
        unsigned attack=stats[1],defense=stats[2],speed=stats[5],special=stats[3];
        unsigned bits=(attack>>3)*8+(defense>>3)*4+(speed>>3)*2+(special>>3);
        out->type=hidden_power_gen2_type((uint8_t)attack,(uint8_t)defense);
        out->power=(uint8_t)((5*bits+(special&3))/2+31);
    } else {
        /* Game bit order is HP/ATK/DEF/SPE/SPA/SPD, not the displayed order. */
        const unsigned order[]={0,1,2,5,3,4};unsigned type_bits=0,power_bits=0;
        for(unsigned i=0;i<6;i++) {
            type_bits|=(unsigned)(stats[order[i]]&1)<<i;
            power_bits|=(unsigned)((stats[order[i]]>>1)&1)<<i;
        }
        out->type=types[15*type_bits/63];
        out->power=(uint8_t)(40*power_bits/63+30);
    }
    return true;
}

#include "move_effectiveness.h"
#include "hidden_power.h"
enum { RULE_UNKNOWN, RULE_STATUS, RULE_CHART, RULE_FIXED, RULE_CONDITIONAL,
       RULE_TYPELESS, RULE_HIDDEN };
#include "gen2_move_data.h"
static bool valid(enum pokemon_type t) { return t>=TYPE_NORMAL && t<=TYPE_STEEL; }
static unsigned single(enum pokemon_type attack, enum pokemon_type defense, bool identified)
{
    if(identified && defense==TYPE_GHOST && (attack==TYPE_NORMAL || attack==TYPE_FIGHTING))
        return 4;
    for(unsigned i=0;i<sizeof(matchups)/sizeof(*matchups);i++)
        if(matchups[i].attack==attack && matchups[i].defense==defense)
            return matchups[i].quarters;
    return 4;
}
int gen2_type_factor(enum pokemon_type attack, enum pokemon_type first,
                     enum pokemon_type second, bool identified)
{
    if(!valid(attack) || !valid(first) || (second!=TYPE_NONE && !valid(second))) return -1;
    unsigned q=single(attack,first,identified);
    if(second!=TYPE_NONE && second!=first) q=q*single(attack,second,identified)/4;
    return (int)q;
}
enum move_effectiveness gen2_move_effectiveness(uint8_t move, uint8_t dvs,
    enum pokemon_type first, enum pokemon_type second, bool identified)
{
    if(!move || move>251 || !valid(first) || (second!=TYPE_NONE && !valid(second)))
        return MOVE_UNKNOWN;
    const struct move_data *m=&moves[move];
    if(m->rule==RULE_UNKNOWN) return MOVE_UNKNOWN;
    if(m->rule==RULE_STATUS) return MOVE_STATUS;
    if(m->rule==RULE_TYPELESS) return MOVE_NEUTRAL;
    enum pokemon_type type=m->type;
    if(m->rule==RULE_HIDDEN) {
        type=hidden_power_gen2_type(dvs>>4,dvs&15);
        if(type==TYPE_NONE) return MOVE_UNKNOWN;
    }
    int factor=gen2_type_factor(type,first,second,identified);
    if(factor<0) return MOVE_UNKNOWN;
    if(!factor) return MOVE_IMMUNE;
    if(m->rule==RULE_CONDITIONAL) return MOVE_UNKNOWN;
    if(m->rule==RULE_FIXED) return MOVE_NEUTRAL;
    return factor>4 ? MOVE_SUPER : factor<4 ? MOVE_RESISTED : MOVE_NEUTRAL;
}

#include "multigen_effectiveness.h"
#include "hidden_power.h"
enum { R_UNKNOWN,R_STATUS,R_CHART,R_FIXED,R_CONDITIONAL,R_DYNAMIC,R_HIDDEN,R_TYPELESS };
struct battle_move_fact {enum pokemon_type type;uint8_t rule,power;};
struct battle_matchup {enum pokemon_type attack,defense;uint8_t factor;};
#include "gen1_battle_data.h"
#include "gen3_battle_data.h"
static bool valid(enum pokemon_type t,unsigned gen)
{
    return t>=TYPE_NORMAL && t<=(gen==1?TYPE_DRAGON:TYPE_STEEL);
}
static int factor(const struct battle_matchup *table,size_t count,enum pokemon_type attack,
    enum pokemon_type first,enum pokemon_type second,bool identified,unsigned gen)
{
    if(!valid(attack,gen) || !valid(first,gen) || (second!=TYPE_NONE && !valid(second,gen)))return -1;
    unsigned q=4;
    for(size_t i=0;i<count;i++) {
        const struct battle_matchup *m=&table[i];
        if(m->attack!=attack || (m->defense!=first && m->defense!=second))continue;
        if(identified && m->defense==TYPE_GHOST && (attack==TYPE_NORMAL || attack==TYPE_FIGHTING))continue;
        /* Each matching pair occurs once even when both stored types coincide. */
        q=q*m->factor/4;
    }
    return (int)q;
}
int gen1_type_factor(enum pokemon_type a,enum pokemon_type f,enum pokemon_type s)
{return factor(gen1_matchups,sizeof(gen1_matchups)/sizeof(*gen1_matchups),a,f,s,false,1);}
int gen3_type_factor(enum pokemon_type a,enum pokemon_type f,enum pokemon_type s,bool identified)
{return factor(gen3_matchups,sizeof(gen3_matchups)/sizeof(*gen3_matchups),a,f,s,identified,3);}
static enum move_effectiveness classify(int q)
{return q<0?MOVE_UNKNOWN:q==0?MOVE_IMMUNE:q>4?MOVE_SUPER:q<4?MOVE_RESISTED:MOVE_NEUTRAL;}
enum move_effectiveness gen1_move_effectiveness(uint16_t move,enum pokemon_type f,enum pokemon_type s)
{
    if(!move || move>=166 || !valid(f,1) || (s!=TYPE_NONE && !valid(s,1)))return MOVE_UNKNOWN;
    const struct battle_move_fact *m=&gen1_moves[move];
    if(m->rule==R_DYNAMIC)return MOVE_UNKNOWN;
    if(m->rule==R_STATUS)return MOVE_STATUS;
    /* Red/Blue skip AdjustDamageForMoveType for special damage and Super Fang.
     * Seismic Toss/Night Shade therefore bypass Ghost/Normal immunities. */
    if(m->rule==R_FIXED)return MOVE_NEUTRAL;
    int q=gen1_type_factor(m->type,f,s);
    if(q==0)return MOVE_IMMUNE;
    if(m->rule==R_CONDITIONAL)return MOVE_UNKNOWN;
    return classify(q);
}
static bool sound(uint16_t move)
{
    static const uint16_t ids[]={45,46,47,48,103,173,253,319,320,304};
    for(unsigned i=0;i<sizeof(ids)/sizeof(*ids);i++)if(move==ids[i])return true;
    return false;
}
enum move_effectiveness gen3_move_effectiveness(uint16_t move,const struct gen3_move_context *c)
{
    if(!move || move>=355 || !c || !c->known || c->ambiguous || c->ability>=78 ||
       !valid(c->first,3) || (c->second!=TYPE_NONE && !valid(c->second,3)))return MOVE_UNKNOWN;
    const struct battle_move_fact *m=&gen3_moves[move];
    if(m->rule==R_DYNAMIC)return MOVE_UNKNOWN;
    if(m->rule==R_TYPELESS)return MOVE_NEUTRAL; /* Struggle bypasses chart/Wonder Guard. */
    enum pokemon_type type=m->type;
    if(m->rule==R_HIDDEN){struct hidden_power hp;if(!hidden_power_calculate(3,c->ivs,&hp))return MOVE_UNKNOWN;type=hp.type;}
    if(c->ability==43 && sound(move))return MOVE_IMMUNE;
    if(c->ability==18 && type==TYPE_FIRE && !c->frozen)return MOVE_IMMUNE;
    if(m->rule==R_STATUS)return MOVE_STATUS;
    if((c->ability==10 && type==TYPE_ELECTRIC) || (c->ability==11 && type==TYPE_WATER) ||
       (c->ability==26 && type==TYPE_GROUND))return MOVE_IMMUNE;
    if(c->ability==5 && (move==12 || move==32 || move==90 || move==329))return MOVE_IMMUNE;
    int q=gen3_type_factor(type,c->first,c->second,c->identified);
    if(q==0)return MOVE_IMMUNE;
    if(c->ability==25 && q<=4)return MOVE_IMMUNE;
    if(m->rule==R_CONDITIONAL)return MOVE_UNKNOWN;
    if(m->rule==R_FIXED)return MOVE_NEUTRAL;
    return classify(q);
}

#include "battle_decoder.h"
bool gen2_type_decode(uint8_t raw, enum pokemon_type *type)
{
    static const enum pokemon_type physical[] = {
        TYPE_NORMAL,TYPE_FIGHTING,TYPE_FLYING,TYPE_POISON,TYPE_GROUND,
        TYPE_ROCK,TYPE_NONE,TYPE_BUG,TYPE_GHOST,TYPE_STEEL
    };
    static const enum pokemon_type special[] = {
        TYPE_FIRE,TYPE_WATER,TYPE_GRASS,TYPE_ELECTRIC,TYPE_PSYCHIC,
        TYPE_ICE,TYPE_DRAGON,TYPE_DARK
    };
    enum pokemon_type result=TYPE_NONE;
    if(raw<sizeof(physical)/sizeof(*physical)) result=physical[raw];
    else if(raw>=20 && raw<28) result=special[raw-20];
    if(!type || result==TYPE_NONE) return false;
    *type=result; return true;
}
const char *pokemon_type_name(enum pokemon_type type)
{
    static const char *names[]={"NONE","NORMAL","FIRE","WATER","ELECTRIC","GRASS",
        "ICE","FIGHTING","POISON","GROUND","FLYING","PSYCHIC","BUG","ROCK",
        "GHOST","DRAGON","DARK","STEEL","FAIRY"};
    return type>=TYPE_NONE && type<=TYPE_FAIRY ? names[type] : "UNKNOWN";
}
struct battle_state battle_decode(const struct game_profile *p,
                                 battle_memory_read read, void *context)
{
    struct battle_state s={0};
    if(!p) return s;
    s.status=BATTLE_UNAVAILABLE;
    uint8_t mode, ended, starting, switching;
    if(!read || !read(context,p->battle_mode,&mode,1)) return s;
    if(mode==0) { s.status=BATTLE_OUTSIDE; return s; }
    if(mode!=1 && mode!=2) { s.status=BATTLE_INVALID; return s; }
    if(!read(context,p->battle_ended,&ended,1) ||
       !read(context,p->battle_starting,&starting,1) ||
       !read(context,p->enemy_switching,&switching,1)) return s;
    if(ended) { s.status=BATTLE_OUTSIDE; return s; }
    s.mode=mode;
    if(starting || switching) { s.status=BATTLE_TRANSITION; return s; }
    uint8_t species,level,hp[2],max_hp[2],types[2];
    if(!read(context,p->enemy_species,&species,1) ||
       !read(context,p->enemy_level,&level,1) ||
       !read(context,p->enemy_hp,hp,2) ||
       !read(context,p->enemy_max_hp,max_hp,2) ||
       !read(context,p->enemy_type1,&types[0],1) ||
       !read(context,p->enemy_type2,&types[1],1)) return s;
    unsigned current=(unsigned)hp[0]*256+hp[1], max=(unsigned)max_hp[0]*256+max_hp[1];
    if(!current) { s.status=BATTLE_TRANSITION; return s; }
    enum pokemon_type t1,t2;
    if(!species || species>251 || !level || level>100 || !max || max>999 || current>max ||
       !gen2_type_decode(types[0],&t1) || !gen2_type_decode(types[1],&t2)) {
        s.status=BATTLE_INVALID; return s;
    }
    s.status=BATTLE_ACTIVE; s.species=species;
    s.type1=t1; s.type2=t1==t2 ? TYPE_NONE : t2;
    s.raw_type1=types[0]; s.raw_type2=types[1];
    return s;
}
bool battle_state_equal(const struct battle_state *a, const struct battle_state *b)
{
    return a->status==b->status && a->mode==b->mode && a->species==b->species &&
        a->type1==b->type1 && a->type2==b->type2 && a->raw_type1==b->raw_type1 &&
        a->raw_type2==b->raw_type2;
}

#include "battle_decoder.h"
#include "move_effectiveness.h"
#include <string.h>
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
static bool labels_match(const struct menu_label *labels, battle_memory_read read, void *context)
{
    for (unsigned i = 0; i < 4; ++i) {
        const struct menu_label *label = &labels[i];
        uint8_t tiles[5];
        if (!label->size || label->size > sizeof(tiles) ||
            !read(context, label->address, tiles, label->size) ||
            memcmp(tiles, label->tiles, label->size))
            return false;
    }
    return true;
}
static bool fight_menu_visible(const struct game_profile *p, battle_memory_read read, void *context)
{
    uint8_t type, geometry[4], offsets;
    if (!read(context, p->move_menu_type, &type, 1) || type != 0 ||
        !read(context, p->move_geometry, geometry, sizeof(geometry)) ||
        !read(context, p->move_cursor_offsets, &offsets, 1) ||
        geometry[0] != p->move_origin_y || geometry[1] != p->move_origin_x ||
        !geometry[2] || geometry[2] > p->move_max_rows || geometry[3] != 1 ||
        offsets != p->move_offset)
        return false;
    return labels_match(p->move_menu_labels, read, context);
}
static bool main_menu_visible(const struct game_profile *p, battle_memory_read read, void *context)
{
    uint8_t pointer[2], bank;
    if (!read(context, p->menu_data_pointer, pointer, sizeof(pointer)) ||
        !read(context, p->menu_data_bank, &bank, 1) || bank != p->main_menu_bank ||
        ((unsigned)pointer[0] | (unsigned)pointer[1] << 8) != p->main_menu_pointer)
        return false;
    /* Metadata can outlive a draw/restore. Require all four rendered labels too. */
    return labels_match(p->main_menu_labels, read, context);
}
struct battle_state battle_decode(const struct game_profile *p,
                                 battle_memory_read read, void *context)
{
    struct battle_state s={0};
    if(!p) return s;
    s.training=training_stats_decode(p,read,context);
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
    s.main_menu=main_menu_visible(p,read,context);
    s.fight_menu=fight_menu_visible(p,read,context);
    if(s.fight_menu) {
        uint8_t ids[4], pp[4], dvs, disabled, identified, rows;
        if(read(context,p->player_moves,ids,4) && read(context,p->player_pp,pp,4) &&
           read(context,p->player_dvs,&dvs,1) && read(context,p->player_disable,&disabled,1) &&
           read(context,p->enemy_substatus1,&identified,1) &&
           read(context,p->move_geometry+2,&rows,1)) {
            unsigned count=0;
            while(count<4 && ids[count] && ids[count]<=251) count++;
            bool valid=count==rows;
            for(unsigned i=count;i<4;i++) if(ids[i]) valid=false;
            if(valid) for(unsigned i=0;i<count;i++) {
                s.moves[i]=ids[i];
                s.effectiveness[i]=!(pp[i]&0x3f) || ((disabled&0x0f) && (disabled>>4)==i+1) ?
                    MOVE_UNUSABLE : gen2_move_effectiveness(ids[i],dvs,t1,s.type2,(identified&8)!=0);
            }
        }
    }
    return s;
}
bool battle_state_equal(const struct battle_state *a, const struct battle_state *b)
{
    return a->status==b->status && a->mode==b->mode && a->species==b->species &&
        a->type1==b->type1 && a->type2==b->type2 && a->raw_type1==b->raw_type1 &&
        a->raw_type2==b->raw_type2 && a->main_menu==b->main_menu && a->fight_menu==b->fight_menu &&
        !memcmp(a->moves,b->moves,sizeof(a->moves)) &&
        !memcmp(a->effectiveness,b->effectiveness,sizeof(a->effectiveness)) &&
        training_stats_equal(&a->training,&b->training);
}

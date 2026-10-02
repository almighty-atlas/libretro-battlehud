#ifndef BATTLEHUD_MULTIGEN_EFFECTIVENESS_H
#define BATTLEHUD_MULTIGEN_EFFECTIVENESS_H
#include "battle_decoder.h"
int gen1_type_factor(enum pokemon_type attack,enum pokemon_type first,enum pokemon_type second);
enum move_effectiveness gen1_move_effectiveness(uint16_t move,enum pokemon_type first,enum pokemon_type second);
struct gen3_move_context {
    enum pokemon_type first,second;
    uint8_t ability; /* Actual battle ability, including Skill Swap/Trace. */
    bool identified,frozen,known,ambiguous;
    uint8_t ivs[6]; /* HP/ATK/DEF/SPA/SPD/SPE. */
};
int gen3_type_factor(enum pokemon_type attack,enum pokemon_type first,enum pokemon_type second,bool identified);
enum move_effectiveness gen3_move_effectiveness(uint16_t move,const struct gen3_move_context *context);
#endif

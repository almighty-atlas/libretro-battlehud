#ifndef BATTLEHUD_MOVE_EFFECTIVENESS_H
#define BATTLEHUD_MOVE_EFFECTIVENESS_H
#include "battle_decoder.h"
/* Type-only damage factor in quarters: 0/1/2/4/8/16; -1 invalid. */
int gen2_type_factor(enum pokemon_type attack, enum pokemon_type first,
                     enum pokemon_type second, bool identified);
enum move_effectiveness gen2_move_effectiveness(uint8_t move, uint8_t attack_defense_dvs,
    enum pokemon_type first, enum pokemon_type second, bool identified);
#endif

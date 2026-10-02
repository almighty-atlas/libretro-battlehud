#ifndef BATTLEHUD_HIDDEN_POWER_H
#define BATTLEHUD_HIDDEN_POWER_H
#include "battle_decoder.h"
struct hidden_power {
    enum pokemon_type type;
    uint8_t power; /* Base power, before STAB, stats or battle modifiers. */
};
/* Stats order is HP/ATK/DEF/SPA/SPD/SPE, matching training_stats.dv. */
bool hidden_power_calculate(unsigned generation,const uint8_t stats[6],struct hidden_power *out);
enum pokemon_type hidden_power_gen2_type(uint8_t attack,uint8_t defense);
#endif

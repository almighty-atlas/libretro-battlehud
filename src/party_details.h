#ifndef BATTLEHUD_PARTY_DETAILS_H
#define BATTLEHUD_PARTY_DETAILS_H
#include "training_stats.h"
/* Displayed stat order HP/ATK/DEF/SPA/SPD/SPE; +1/-1 represent 110%/90%. */
int gen3_nature_effect(uint8_t nature,unsigned stat);
const char *gen3_nature_name(uint8_t nature);
bool gen3_ability_read(const struct game_profile *p,training_memory_read read,void *ctx,
                       uint16_t species,uint8_t slot,uint8_t *ability,char name[13]);
#endif

#ifndef BATTLEHUD_MULTIGEN_BATTLE_H
#define BATTLEHUD_MULTIGEN_BATTLE_H
#include "battle_decoder.h"
struct battle_state multigen_battle_decode(const struct game_profile *profile,battle_memory_read read,void *ctx);
bool gen3_type_decode(uint8_t raw,enum pokemon_type *type);
#endif

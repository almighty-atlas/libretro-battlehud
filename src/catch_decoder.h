#ifndef BATTLEHUD_CATCH_DECODER_H
#define BATTLEHUD_CATCH_DECODER_H
#include "battle_decoder.h"
struct catch_hint catch_decode(const struct game_profile *,battle_memory_read,void *,const struct battle_state *);
#endif

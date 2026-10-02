#ifndef BATTLEHUD_CATCH_CHANCE_H
#define BATTLEHUD_CATCH_CHANCE_H
#include <stdbool.h>
#include <stdint.h>
struct catch_hint {bool visible,known,master;uint8_t ball;uint16_t permyriad;};
/* Crystal's modified byte catch rate, including original integer overflow behavior. */
bool crystal_catch_chance(uint8_t rate,uint16_t hp,uint16_t max_hp,uint8_t status,
                          bool level_ball,uint16_t *permyriad);
const char *crystal_ball_name(uint8_t ball);
#endif

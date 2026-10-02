#ifndef BATTLEHUD_TRAINING_STATS_H
#define BATTLEHUD_TRAINING_STATS_H
#include "game_profile.h"
#include <stdbool.h>
#include <stddef.h>
/* Display order: HP, Attack, Defense, Special Attack, Special Defense, Speed. */
struct training_stats {
    bool visible;
    uint8_t slot, species, dv[6];
    uint16_t ev[6]; /* Gen 2 raw stat experience, NOT modern 0..252 EVs. */
};
typedef bool (*training_memory_read)(void *, size_t, void *, size_t);
struct training_stats training_stats_decode(const struct game_profile *profile,
                                           training_memory_read read, void *context);
bool training_stats_equal(const struct training_stats *a,const struct training_stats *b);
#endif

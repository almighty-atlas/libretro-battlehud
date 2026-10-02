#ifndef BATTLEHUD_TRAINING_STATS_H
#define BATTLEHUD_TRAINING_STATS_H
#include "game_profile.h"
#include <stdbool.h>
#include <stddef.h>
/* Display order: HP, Attack, Defense, Special Attack, Special Defense, Speed. */
struct training_stats {
    bool visible;
    uint8_t slot, generation;
    uint16_t species;
    uint8_t dv[6]; /* DVs in Gen 1/2, IVs in Gen 3. */
    uint16_t ev[6]; /* Raw stat experience in Gen 1/2, EVs in Gen 3. */
    bool nature_known, ability_known;
    uint8_t nature, ability, ability_slot;
    char ability_name[13];
    uint8_t level, base[6];
    bool bonus_known, identity_known, gain_known;
    uint8_t identity[32]; /* Conservative party identity; never a slot alone. */
    uint16_t gain[6]; /* Observed raw training change, not a prediction. */
    bool extras_known, gender_known, shiny;
    uint8_t friendship, pokerus, gender; /* Gender: 0 male, 1 female, 2 genderless. */
};
typedef bool (*training_memory_read)(void *, size_t, void *, size_t);
struct training_stats training_stats_decode(const struct game_profile *profile,
                                           training_memory_read read, void *context);
/* Decode a party member without depending on a summary screen. Eggs are excluded. */
struct training_stats training_party_mon(const struct game_profile *profile,
                                        training_memory_read read, void *context, unsigned slot);
bool training_stats_equal(const struct training_stats *a,const struct training_stats *b);
#endif

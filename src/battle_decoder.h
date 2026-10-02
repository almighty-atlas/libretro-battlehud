#ifndef BATTLEHUD_BATTLE_DECODER_H
#define BATTLEHUD_BATTLE_DECODER_H
#include "game_profile.h"
#include "training_stats.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
enum pokemon_type {
    TYPE_NONE, TYPE_NORMAL, TYPE_FIRE, TYPE_WATER, TYPE_ELECTRIC, TYPE_GRASS,
    TYPE_ICE, TYPE_FIGHTING, TYPE_POISON, TYPE_GROUND, TYPE_FLYING,
    TYPE_PSYCHIC, TYPE_BUG, TYPE_ROCK, TYPE_GHOST, TYPE_DRAGON, TYPE_DARK,
    TYPE_STEEL, TYPE_FAIRY
};
enum battle_status { BATTLE_UNSUPPORTED, BATTLE_OUTSIDE, BATTLE_TRANSITION,
                     BATTLE_ACTIVE, BATTLE_UNAVAILABLE, BATTLE_INVALID };
enum move_effectiveness { MOVE_UNKNOWN, MOVE_SUPER, MOVE_RESISTED, MOVE_NEUTRAL,
                         MOVE_IMMUNE, MOVE_STATUS, MOVE_UNUSABLE };
struct battle_state {
    enum battle_status status;
    uint8_t mode;
    uint16_t species;
    enum pokemon_type type1, type2;
    uint8_t raw_type1, raw_type2;
    bool main_menu, fight_menu; /* Presentation eligibility, independent of combatant. */
    uint8_t moves[4], effectiveness[4]; /* FIGHT list order; zero slots stay empty. */
    struct training_stats training;
};
typedef bool (*battle_memory_read)(void *context, size_t address, void *out, size_t size);
struct battle_state battle_decode(const struct game_profile *profile,
                                 battle_memory_read read, void *context);
bool gen2_type_decode(uint8_t raw, enum pokemon_type *type);
const char *pokemon_type_name(enum pokemon_type type);
bool battle_state_equal(const struct battle_state *a, const struct battle_state *b);
#endif

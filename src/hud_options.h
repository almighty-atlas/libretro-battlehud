#ifndef BATTLEHUD_OPTIONS_H
#define BATTLEHUD_OPTIONS_H
#include "libretro.h"
#define HUD_TYPES 1u
#define HUD_MOVES 2u
#define HUD_TRAINING 4u
#define HUD_POWER 8u
#define HUD_DETAILS 16u
#define HUD_COMPACT 32u
#define HUD_BONUS 64u
#define HUD_GAINS 128u
#define HUD_CATCH 256u
#define HUD_EXTRA_VIEW 512u
#define HUD_DEFAULT (HUD_CATCH|HUD_TYPES|HUD_MOVES|HUD_TRAINING|HUD_POWER|HUD_DETAILS)
/* Registration copies keep backend definitions and translations intact. */
bool hud_options_register(retro_environment_t cb, unsigned cmd, void *data);
void hud_options_fallback(retro_environment_t cb);
unsigned hud_options_read(retro_environment_t cb);
void hud_options_clear(void);
#endif

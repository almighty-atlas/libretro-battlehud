#ifndef BATTLEHUD_TYPE_HUD_H
#define BATTLEHUD_TYPE_HUD_H
#include "libretro.h"
#include "battle_decoder.h"
#include <stddef.h>
#include <stdint.h>
struct type_hud {
    uint8_t *clean, *output;
    size_t clean_capacity, output_capacity;
    unsigned width, height;
    size_t pitch;
    enum retro_pixel_format format;
    bool has_frame, has_presented;
    struct battle_state presented;
};
/* Software frames only. The original core frame is never modified.
 * Retains a clean frame so changed models can replace NULL duplicate frames. */
const void *type_hud_draw(struct type_hud *hud, const struct battle_state *state,
    const void *frame, unsigned width, unsigned height, size_t pitch,
    enum retro_pixel_format format);
void type_hud_clear(struct type_hud *hud);
#endif

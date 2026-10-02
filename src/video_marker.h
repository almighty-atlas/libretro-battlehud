#ifndef BATTLEHUD_VIDEO_MARKER_H
#define BATTLEHUD_VIDEO_MARKER_H

#include "libretro.h"
#include <stddef.h>
#include <stdint.h>

struct video_marker {
    uint8_t *buffer;
    size_t capacity;
};

/* Returns the original frame on unsupported layouts or allocation failure. */
const void *video_marker_draw(struct video_marker *marker, const void *frame,
    unsigned width, unsigned height, size_t pitch, enum retro_pixel_format format);
void video_marker_clear(struct video_marker *marker);

#endif

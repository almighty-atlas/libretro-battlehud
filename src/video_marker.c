#include "video_marker.h"

#include <stdlib.h>
#include <string.h>

const void *video_marker_draw(struct video_marker *marker, const void *frame,
    unsigned width, unsigned height, size_t pitch, enum retro_pixel_format format)
{
    size_t bpp, bytes, row_bytes;
    unsigned marker_width, marker_height, left;
    uint32_t white;

    /* NULL means duplicate frame; hardware sentinels are never CPU buffers. */
    if (!frame || frame == RETRO_HW_FRAME_BUFFER_VALID || !width || !height ||
        width > 4096 || height > 4096)
        return frame;

    switch (format) {
    case RETRO_PIXEL_FORMAT_XRGB8888: bpp = 4; white = 0x00ffffff; break;
    case RETRO_PIXEL_FORMAT_RGB565: bpp = 2; white = 0xffff; break;
    case RETRO_PIXEL_FORMAT_0RGB1555: bpp = 2; white = 0x7fff; break;
    default: return frame;
    }
    row_bytes = (size_t)width * bpp;
    if (pitch < row_bytes || pitch > SIZE_MAX / height)
        return frame;
    bytes = pitch * height;
    if (bytes > 128u * 1024u * 1024u)
        return frame;
    if (bytes > marker->capacity) {
        uint8_t *buffer = realloc(marker->buffer, bytes);
        if (!buffer)
            return frame;
        marker->buffer = buffer;
        marker->capacity = bytes;
    }

    /* The core owns a const frame. Reuse one scratch buffer, never write to RAM. */
    /* Copy visible rows only: the last row's trailing padding may be absent. */
    for (unsigned y = 0; y < height; ++y)
        memcpy(marker->buffer + y * pitch, (const uint8_t *)frame + y * pitch,
               row_bytes);

    marker_width = width < 8 ? width : 8;
    marker_height = height < 8 ? height : 8;
    left = width - marker_width;
    for (unsigned y = 0; y < marker_height; ++y) {
        for (unsigned x = left; x < width; ++x) {
            uint8_t *pixel = marker->buffer + y * pitch + x * bpp;
            if (bpp == 4)
                memcpy(pixel, &white, sizeof(white));
            else {
                uint16_t white16 = (uint16_t)white;
                memcpy(pixel, &white16, sizeof(white16));
            }
        }
    }
    return marker->buffer;
}

void video_marker_clear(struct video_marker *marker)
{
    free(marker->buffer);
    memset(marker, 0, sizeof(*marker));
}

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "video_marker.h"

static void check_format(enum retro_pixel_format format, unsigned width, unsigned height)
{
    struct video_marker marker = {0};
    unsigned bpp = format == RETRO_PIXEL_FORMAT_XRGB8888 ? 4 : 2;
    size_t pitch = width * bpp + 7;
    uint8_t input[100000], original[100000];
    size_t actual_bytes = pitch * (height - 1) + width * bpp;
    assert(actual_bytes <= sizeof(input));
    memset(input, 0x5a, sizeof(input));
    memcpy(original, input, sizeof(input));
    const uint8_t *output = video_marker_draw(&marker, input, width, height, pitch, format);
    assert(output != input);
    assert(memcmp(input, original, sizeof(input)) == 0);
    uint32_t white = format == RETRO_PIXEL_FORMAT_XRGB8888 ? 0xffffff :
        format == RETRO_PIXEL_FORMAT_RGB565 ? 0xffff : 0x7fff;
    unsigned left = width > 8 ? width - 8 : 0;
    for (unsigned y = 0; y < height; ++y) {
        for (unsigned x = 0; x < width; ++x) {
            uint32_t pixel = 0;
            memcpy(&pixel, output + y * pitch + x * bpp, bpp);
            if (y < 8 && x >= left)
                assert(pixel == white);
            else
                assert(memcmp(output + y * pitch + x * bpp,
                              input + y * pitch + x * bpp, bpp) == 0);
        }
    }
    /* No allocation when the same dimensions repeat. */
    assert(video_marker_draw(&marker, input, width, height, pitch, format) == output);
    assert(video_marker_draw(&marker, NULL, width, height, pitch, format) == NULL);
    assert(video_marker_draw(&marker, RETRO_HW_FRAME_BUFFER_VALID,
                             width, height, pitch, format) == RETRO_HW_FRAME_BUFFER_VALID);
    assert(video_marker_draw(&marker, input, width, height, width * bpp - 1, format) == input);
    assert(video_marker_draw(&marker, input, width, height, pitch,
                             (enum retro_pixel_format)99) == input);
    assert(video_marker_draw(&marker, input, 0, height, pitch, format) == input);
    assert(video_marker_draw(&marker, input, width, height, SIZE_MAX, format) == input);
    assert(video_marker_draw(&marker, input, width, 5000, pitch, format) == input);
    video_marker_clear(&marker);
    assert(!marker.buffer && !marker.capacity);
    video_marker_clear(&marker);
}

int main(void)
{
    check_format(RETRO_PIXEL_FORMAT_0RGB1555, 160, 144);
    check_format(RETRO_PIXEL_FORMAT_RGB565, 160, 144);
    check_format(RETRO_PIXEL_FORMAT_XRGB8888, 160, 144);
    check_format(RETRO_PIXEL_FORMAT_RGB565, 3, 2);
    puts("M1 marker formats, clipping, immutable input and fallback tests passed");
    return 0;
}

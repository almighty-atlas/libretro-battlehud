#ifndef BATTLEHUD_MEMORY_VIEW_H
#define BATTLEHUD_MEMORY_VIEW_H
#include "libretro.h"
#include <stdbool.h>
#include <stddef.h>
#define BATTLEHUD_MAX_DESCRIPTORS 256
struct memory_view {
    struct retro_memory_descriptor descriptors[BATTLEHUD_MAX_DESCRIPTORS];
    char spaces[BATTLEHUD_MAX_DESCRIPTORS][9];
    unsigned count;
};
void memory_view_clear(struct memory_view *view);
bool memory_view_capture(struct memory_view *view, const struct retro_memory_map *map);
/* CPU address space only; false leaves destination unchanged. */
bool memory_view_read(const struct memory_view *view, size_t address, void *out, size_t size);
#endif

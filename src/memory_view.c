#include "memory_view.h"
#include <stdint.h>
#include <string.h>

void memory_view_clear(struct memory_view *view)
{
    memset(view, 0, sizeof(*view));
}

bool memory_view_capture(struct memory_view *view, const struct retro_memory_map *map)
{
    memory_view_clear(view);
    if (!map || map->num_descriptors > BATTLEHUD_MAX_DESCRIPTORS ||
        (map->num_descriptors && !map->descriptors))
        return false;
    for (unsigned i = 0; i < map->num_descriptors; ++i) {
        const struct retro_memory_descriptor *d = &map->descriptors[i];
        if (d->addrspace) {
            size_t n = strnlen(d->addrspace, 9);
            if (n > 8) {
                memory_view_clear(view);
                return false;
            }
            memcpy(view->spaces[i], d->addrspace, n);
        }
        view->descriptors[i] = *d;
        view->descriptors[i].addrspace = view->spaces[i];
    }
    view->count = map->num_descriptors;
    return true;
}

static const unsigned char *resolve(const struct memory_view *view, size_t address)
{
    for (unsigned i = 0; i < view->count; ++i) {
        const struct retro_memory_descriptor *d = &view->descriptors[i];
        if (d->addrspace[0])
            continue;
        if (d->select) {
            if ((address & d->select) != (d->start & d->select))
                continue;
        } else if (address < d->start || address - d->start >= d->len) {
            continue;
        }
        /* First claiming descriptor wins. Never guess bank/mirror semantics. */
        if (!d->ptr || d->disconnect || !d->len || address < d->start)
            return NULL;
        size_t index = address - d->start;
        if (index >= d->len || d->offset > SIZE_MAX - index)
            return NULL;
        size_t offset = d->offset + index;
        if ((uintptr_t)d->ptr > UINTPTR_MAX - offset)
            return NULL;
        return (const unsigned char *)d->ptr + offset;
    }
    return NULL;
}

bool memory_view_read(const struct memory_view *view, size_t address, void *out, size_t size)
{
    if (!out || !size || size > 1024 * 1024 || address > SIZE_MAX - (size - 1))
        return false;
    /* Validate the entire range before copying, including descriptor boundaries. */
    for (size_t i = 0; i < size; ++i)
        if (!resolve(view, address + i))
            return false;
    for (size_t i = 0; i < size; ++i)
        ((unsigned char *)out)[i] = *resolve(view, address + i);
    return true;
}

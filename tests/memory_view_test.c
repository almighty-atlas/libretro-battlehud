#ifdef NDEBUG
#undef NDEBUG
#endif
#include "memory_view.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
int main(void)
{
    struct memory_view v = {0};
    unsigned char ram[] = {1,2,3,4,5,6,7,8}, out[4] = {9,9,9,9};
    char space[] = "RAM";
    struct retro_memory_descriptor d[] = {
        {0,ram,1,0xC000,0,0,4,NULL},
        {0,ram,5,0xC004,0,0,2,NULL},
        {0,ram,0,0xA000,(size_t)~0x1FFF,0,8,NULL},
        {0,ram,0,0xC006,0,0,2,space}
    };
    struct retro_memory_map m = {d,4};
    assert(memory_view_capture(&v,&m));
    d[0].ptr = NULL; space[0] = 'X'; /* Capture owns descriptors and labels. */
    assert(memory_view_read(&v,0xC002,out,4));
    assert(!memcmp(out,(unsigned char[]){4,5,6,7},4));
    assert(memory_view_read(&v,0xA000,out,1) && out[0] == 1);
    memset(out,9,sizeof(out));
    assert(!memory_view_read(&v,0xC004,out,4));
    assert(!memcmp(out,(unsigned char[]){9,9,9,9},4));
    assert(!memory_view_read(&v,0xA008,out,1)); /* Unsupported mirror: no guess. */
    assert(!memory_view_read(&v,SIZE_MAX,out,2));
    assert(!memory_view_read(&v,0xC000,NULL,1));
    assert(!memory_view_read(&v,0xC000,out,0));
    d[0].ptr = ram; d[0].disconnect = 1;
    assert(memory_view_capture(&v,&m));
    assert(!memory_view_read(&v,0xC000,out,1));
    d[0].disconnect = 0; d[0].offset = SIZE_MAX;
    assert(memory_view_capture(&v,&m));
    assert(!memory_view_read(&v,0xC001,out,1));
    /* A NULL first descriptor must not expose a later overlapping mapping. */
    d[0] = (struct retro_memory_descriptor){0,NULL,0,0xC000,0,0,4,NULL};
    d[1] = (struct retro_memory_descriptor){0,ram,0,0xC000,0,0,4,NULL};
    assert(memory_view_capture(&v,&m));
    assert(!memory_view_read(&v,0xC000,out,1));
    m.num_descriptors = BATTLEHUD_MAX_DESCRIPTORS + 1;
    assert(!memory_view_capture(&v,&m) && v.count == 0);
    m.num_descriptors = 1; m.descriptors = NULL;
    assert(!memory_view_capture(&v,&m));
    m.num_descriptors = 0;
    assert(memory_view_capture(&v,&m));
    assert(!memory_view_read(&v,0xC000,out,1));
    assert(!memcmp(ram,(unsigned char[]){1,2,3,4,5,6,7,8},8));
    memory_view_clear(&v);
    puts("M2 read-only memory adapter passed");
    return 0;
}

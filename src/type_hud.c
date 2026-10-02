#include "type_hud.h"
#include <stdlib.h>
#include <string.h>

/* Original 5x7 uppercase bitmap font; each row uses the low five bits. */
static const uint8_t font[26][7] = {
    {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
    {14,17,16,16,16,17,14}, {30,17,17,17,17,17,30},
    {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
    {14,17,16,23,17,17,15}, {17,17,17,31,17,17,17},
    {14,4,4,4,4,4,14}, {7,2,2,2,18,18,12},
    {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17},
    {14,17,17,17,17,17,14}, {30,17,17,30,16,16,16},
    {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4},
    {17,17,17,17,17,17,14}, {17,17,17,17,17,10,4},
    {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31}
};
static uint32_t color(enum pokemon_type type)
{
    static const uint32_t colors[] = {
        0,0x606060,0xa53c23,0x345a9c,0x866600,0x427033,
        0x286b76,0x913333,0x71378e,0x856332,0x5c5189,
        0x96365f,0x526f21,0x736127,0x514577,0x514096,
        0x493e39,0x57626d,0x875172
    };
    return colors[type];
}
static void pixel(uint8_t *out, size_t pitch, size_t bpp, unsigned x, unsigned y,
                  enum retro_pixel_format format, uint32_t rgb)
{
    uint8_t *p=out+(size_t)y*pitch+(size_t)x*bpp;
    if(format==RETRO_PIXEL_FORMAT_XRGB8888) memcpy(p,&rgb,4);
    else {
        unsigned r=(rgb>>16)&255, g=(rgb>>8)&255, b=rgb&255;
        uint16_t packed=format==RETRO_PIXEL_FORMAT_RGB565 ?
            (uint16_t)((r>>3)<<11 | (g>>2)<<5 | (b>>3)) :
            (uint16_t)((r>>3)<<10 | (g>>3)<<5 | (b>>3));
        memcpy(p,&packed,2);
    }
}
static bool valid_type(enum pokemon_type type) { return type>=TYPE_NORMAL && type<=TYPE_FAIRY; }
static bool fits(const struct battle_state *s, unsigned width, unsigned height)
{
    if(s->status!=BATTLE_ACTIVE || (!s->main_menu && !s->fight_menu) || !s->species || !valid_type(s->type1) ||
       (s->type2!=TYPE_NONE && !valid_type(s->type2))) return false;
    unsigned rows=s->type2!=TYPE_NONE && s->type2!=s->type1 ? 2 : 1;
    if(height < 2+rows*13+(rows-1)*2+2) return false;
    for(unsigned i=0;i<rows;i++) {
        enum pokemon_type type=i?s->type2:s->type1;
        if(width < strlen(pokemon_type_name(type))*6+7+4) return false;
    }
    return true;
}
static void badge(struct type_hud *h, size_t bpp, enum pokemon_type type, unsigned top)
{
    const char *text=pokemon_type_name(type);
    unsigned len=(unsigned)strlen(text), width=len*6+7, left=h->width-2-width;
    for(unsigned y=0;y<13;y++) for(unsigned x=0;x<width;x++) {
        uint32_t rgb=(y==0 || y==12 || x==0 || x==width-1)?0x101010:color(type);
        pixel(h->output,h->pitch,bpp,left+x,top+y,h->format,rgb);
    }
    for(unsigned i=0;i<len;i++) for(unsigned y=0;y<7;y++) for(unsigned x=0;x<5;x++)
        if(font[(unsigned)(text[i]-'A')][y] & (1u<<(4-x)))
            pixel(h->output,h->pitch,bpp,left+4+i*6+x,top+3+y,h->format,0xffffff);
}
static bool reserve(uint8_t **buffer, size_t *capacity, size_t bytes)
{
    if(bytes<=*capacity) return true;
    uint8_t *next=realloc(*buffer,bytes);
    if(!next) return false;
    *buffer=next; *capacity=bytes; return true;
}
const void *type_hud_draw(struct type_hud *h, const struct battle_state *state,
    const void *frame, unsigned width, unsigned height, size_t pitch,
    enum retro_pixel_format format)
{
    struct battle_state empty={0};
    if(!state) state=&empty;
    size_t bpp=format==RETRO_PIXEL_FORMAT_XRGB8888?4:2;
    if(frame==RETRO_HW_FRAME_BUFFER_VALID || !width || !height || width>4096 || height>4096 ||
       (format!=RETRO_PIXEL_FORMAT_XRGB8888 && format!=RETRO_PIXEL_FORMAT_RGB565 &&
        format!=RETRO_PIXEL_FORMAT_0RGB1555) || pitch<(size_t)width*bpp ||
       pitch>SIZE_MAX/height || pitch*height>32u*1024u*1024u) {
        h->has_frame=false; h->has_presented=false;
        return frame;
    }
    size_t bytes=pitch*height, row_bytes=(size_t)width*bpp;
    if(!frame) {
        if(!h->has_frame || h->width!=width || h->height!=height || h->pitch!=pitch ||
           h->format!=format) {
            h->has_frame=false; h->has_presented=false;
            return NULL;
        }
        if(h->has_presented && battle_state_equal(&h->presented,state)) return NULL;
    } else {
        if(!reserve(&h->clean,&h->clean_capacity,bytes)) {
            h->has_frame=false; h->has_presented=false; return frame;
        }
        for(unsigned y=0;y<height;y++)
            memcpy(h->clean+(size_t)y*pitch,(const uint8_t *)frame+(size_t)y*pitch,row_bytes);
        h->width=width; h->height=height; h->pitch=pitch; h->format=format; h->has_frame=true;
    }
    const void *result=frame?frame:h->clean;
    bool active=fits(state,width,height);
    if(active && reserve(&h->output,&h->output_capacity,bytes)) {
        for(unsigned y=0;y<height;y++)
            memcpy(h->output+(size_t)y*pitch,h->clean+(size_t)y*pitch,row_bytes);
        badge(h,bpp,state->type1,2);
        if(state->type2!=TYPE_NONE && state->type2!=state->type1) badge(h,bpp,state->type2,17);
        result=h->output;
    } else if(active) {
        /* Allocation failure passes a clean frame; retry if a duplicate arrives. */
        h->has_presented=false;
        return result;
    }
    h->presented=*state; h->has_presented=true;
    return result;
}
void type_hud_clear(struct type_hud *h)
{
    free(h->clean); free(h->output); memset(h,0,sizeof(*h));
}

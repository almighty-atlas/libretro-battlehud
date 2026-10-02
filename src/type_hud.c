#include "type_hud.h"
#include <stdlib.h>
#include <string.h>

/* Original 8x8 type silhouettes, indexed by normalized type (no ROM art). */
static const uint8_t icons[18][8] = {
    {0x18,0x3c,0x7e,0xff,0xff,0x7e,0x3c,0x18}, /* normal: diamond */
    {0x10,0x10,0x58,0x5c,0x7e,0x76,0x3c,0x18}, /* fire */
    {0x10,0x18,0x3c,0x3c,0x7e,0x7a,0x3c,0x18}, /* water */
    {0x0c,0x18,0x30,0x7e,0x0c,0x18,0x30,0x20}, /* electric */
    {0x03,0x0f,0x1f,0x3d,0x7b,0x76,0x6c,0x80}, /* grass */
    {0x24,0x18,0x5a,0x3c,0x3c,0x5a,0x18,0x24}, /* ice */
    {0x78,0xfe,0xaa,0xfe,0x7e,0x3c,0x3c,0x3c}, /* fighting */
    {0x3c,0x7e,0xdb,0xdb,0x7e,0x24,0x3c,0x24}, /* poison */
    {0x00,0x10,0x38,0x7c,0xfe,0x00,0xff,0x00}, /* ground */
    {0x01,0x07,0x1f,0x7e,0xfc,0xf8,0x70,0x20}, /* flying */
    {0x3c,0x42,0x99,0xa5,0xa5,0x99,0x42,0x3c}, /* psychic */
    {0x42,0x24,0x3c,0x7e,0xdb,0x7e,0x3c,0x42}, /* bug */
    {0x00,0x1c,0x3e,0x7b,0xfb,0xe7,0x7e,0x00}, /* rock */
    {0x3c,0x7e,0xdb,0xdb,0xff,0xff,0xdb,0x81}, /* ghost */
    {0x42,0x66,0x7e,0x5a,0x7e,0x3c,0x18,0x18}, /* dragon */
    {0x3c,0x70,0xe0,0xe0,0xe0,0x70,0x3c,0x00}, /* dark */
    {0x18,0x7e,0x42,0xdb,0xdb,0x42,0x7e,0x18}, /* steel */
    {0x66,0xff,0xff,0x7e,0x3c,0x18,0x00,0x00}  /* fairy (future profiles) */
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
    return width>=16 && height>=2+rows*12+(rows-1)*2+2;
}
static void badge(struct type_hud *h, size_t bpp, enum pokemon_type type, unsigned top)
{
    unsigned left=h->width-14;
    for(unsigned y=0;y<12;y++) for(unsigned x=0;x<12;x++) {
        uint32_t rgb=(y==0 || y==11 || x==0 || x==11)?0x101010:color(type);
        pixel(h->output,h->pitch,bpp,left+x,top+y,h->format,rgb);
    }
    for(unsigned y=0;y<8;y++) for(unsigned x=0;x<8;x++)
        if(icons[type-1][y] & (1u<<(7-x)))
            pixel(h->output,h->pitch,bpp,left+2+x,top+2+y,h->format,0xffffff);
}
static void move_hints(struct type_hud *h, size_t bpp, const struct battle_state *s)
{
    /* One 7x7 marker in the unused x=18 tile, next to each 12-character name.
     * Fixed coordinates belong only to the recognized 160x144 Crystal layout. */
    static const uint8_t glyphs[7][7]={
        {14,17,1,2,4,0,4},     /* ? unknown/conditional */
        {4,14,21,4,4,4,4},    /* up: super */
        {4,4,4,4,21,14,4},    /* down: resisted */
        {0,0,31,0,31,0,0},    /* = neutral */
        {17,10,4,10,17,0,0},  /* x immune */
        {0,0,0,31,0,0,0},     /* - status */
        {31,17,17,17,17,17,31} /* box: no PP/disabled */
    };
    static const uint32_t backgrounds[]={0x665000,0x206038,0x825016,0x405060,
                                         0x902c30,0x505050,0x505050};
    if(!s->fight_menu || h->width!=160 || h->height!=144) return;
    for(unsigned i=0;i<4;i++) if(s->moves[i] && s->effectiveness[i]<=MOVE_UNUSABLE) {
        unsigned k=s->effectiveness[i], top=104+i*8;
        for(unsigned y=0;y<7;y++) for(unsigned x=0;x<7;x++) {
            bool ink=x>=1 && x<=5 && (glyphs[k][y] & (1u<<(5-x)));
            pixel(h->output,h->pitch,bpp,144+x,top+y,h->format,
                  ink ? 0xffffff : backgrounds[k]);
        }
    }
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
        if(state->type2!=TYPE_NONE && state->type2!=state->type1) badge(h,bpp,state->type2,16);
        move_hints(h,bpp,state);
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

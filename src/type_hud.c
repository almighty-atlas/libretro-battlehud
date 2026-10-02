#include "type_hud.h"
#include "hidden_power.h"
#include "party_details.h"
#include "training_progress.h"
#include <stdlib.h>
#include <stdio.h>
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
    if(s->training.visible) return s->training.generation==3 ?
        width==240 && height==160 : (s->training.generation==1 || s->training.generation==2) && width==160 && height==144;
    if(s->status!=BATTLE_ACTIVE || (!s->main_menu && !s->fight_menu) || !s->species || !valid_type(s->type1) ||
       (s->type2!=TYPE_NONE && !valid_type(s->type2))) return false;
    if(s->generation==3 && (width!=240 || height!=160)) return false;
    if(s->generation==1 && (width!=160 || height!=144)) return false;
    unsigned rows=s->type2!=TYPE_NONE && s->type2!=s->type1 ? 2 : 1;
    return width>=16 && height>=2+rows*12+(rows-1)*2+2;
}
static void type_tile(struct type_hud *h,size_t bpp,enum pokemon_type type,unsigned left,unsigned top)
{
    for(unsigned y=0;y<12;y++) for(unsigned x=0;x<12;x++) {
        uint32_t rgb=(y==0 || y==11 || x==0 || x==11)?0x101010:color(type);
        pixel(h->output,h->pitch,bpp,left+x,top+y,h->format,rgb);
    }
    for(unsigned y=0;y<8;y++) for(unsigned x=0;x<8;x++)
        if(icons[type-1][y] & (1u<<(7-x)))
            pixel(h->output,h->pitch,bpp,left+2+x,top+2+y,h->format,0xffffff);
}
static void badge(struct type_hud *h,size_t bpp,enum pokemon_type type,unsigned top)
{
    type_tile(h,bpp,type,h->width-14,top);
}
static void move_hints(struct type_hud *h, size_t bpp, const struct battle_state *s)
{
    /* One 7x7 marker in the unused x=18 tile, next to each 12-character name.
     * GB list layouts use x=18. Emerald markers use the outer left border
     * and right-column gap, preserving name windows, both cursors and PP/type. */
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
    if(!s->fight_menu) return;
    bool gba=s->generation==3;
    if(gba ? h->width!=240 || h->height!=160 : h->width!=160 || h->height!=144) return;
    for(unsigned i=0;i<4;i++) if(s->moves[i] && s->effectiveness[i]<=MOVE_UNUSABLE) {
        unsigned k=s->effectiveness[i], top=gba ? 124+(i/2)*16 : 104+i*8;
        unsigned left=gba ? (i%2 ? 153 : 1) : 144;
        for(unsigned y=0;y<7;y++) for(unsigned x=0;x<7;x++) {
            bool ink=x>=1 && x<=5 && (glyphs[k][y] & (1u<<(5-x)));
            pixel(h->output,h->pitch,bpp,left+x,top+y,h->format,
                  ink ? 0xffffff : backgrounds[k]);
        }
    }
}
/* Original 5x7 ASCII glyphs, reused for read-only training values. */
static const uint8_t stats_font[36][7]={
{14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
{30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
{14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
{7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
{17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
{30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
{17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
{17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
{14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
{30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
{14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
{14,17,17,15,1,1,14}
};
static void stats_text(struct type_hud *h,size_t bpp,unsigned left,unsigned top,
                       const char *text,uint32_t rgb)
{
    for(unsigned i=0;text[i];i++) {
        int index=text[i]>='A' && text[i]<='Z' ? text[i]-'A' :
            text[i]>='0' && text[i]<='9' ? text[i]-'0'+26 : -1;
        if(index<0) continue;
        for(unsigned y=0;y<7;y++) for(unsigned x=0;x<5;x++)
            if(stats_font[index][y] & (1u<<(4-x)))
                pixel(h->output,h->pitch,bpp,left+i*6+x,top+y,h->format,rgb);
    }
}
static void hidden_power_row(struct type_hud *h,size_t bpp,const struct training_stats *s,
                             unsigned left,unsigned top)
{
    struct hidden_power hp;
    if(!hidden_power_calculate(s->generation,s->dv,&hp)) return;
    stats_text(h,bpp,left+2,top+4,"HPWR",0x90c4ff);
    type_tile(h,bpp,hp.type,left+28,top);
    char text[8];snprintf(text,sizeof(text),"P %u",(unsigned)hp.power);
    stats_text(h,bpp,left+52,top+4,text,0xffffff);
}
static void nature_arrow(struct type_hud *h,size_t bpp,int effect,unsigned left,unsigned top)
{
    if(!effect) return;
    const uint8_t up[]={4,14,31,4,4,4,4},down[]={4,4,4,4,31,14,4};
    const uint8_t *shape=effect>0 ? up : down;
    for(unsigned y=0;y<7;y++) for(unsigned x=0;x<5;x++) if(shape[y]&(1u<<(4-x)))
        pixel(h->output,h->pitch,bpp,left+x,top+y,h->format,effect>0 ? 0x78e6a0 : 0xff9292);
}
static void party_details_line(struct type_hud *h,size_t bpp,const struct training_stats *s)
{
    const char *nature=s->nature_known ? gen3_nature_name(s->nature) : NULL;
    bool ability=s->ability_known && s->ability<78;
    size_t n=strnlen(s->ability_name,sizeof(s->ability_name));
    if(!n || n>12) ability=false;
    for(size_t i=0;ability && i<n;i++) if((s->ability_name[i]<'A' || s->ability_name[i]>'Z') && s->ability_name[i]!=' ') ability=false;
    if(!nature && !ability) return;
    /* Gap between the original stat windows (end y=103) and EXP area (y=112). */
    for(unsigned y=104;y<112;y++) for(unsigned x=80;x<240;x++)
        pixel(h->output,h->pitch,bpp,x,y,h->format,0x18202c);
    if(nature) {char text[16];snprintf(text,sizeof(text),"N %s",nature);stats_text(h,bpp,82,105,text,0x90c4ff);}
    if(ability) stats_text(h,bpp,162,105,s->ability_name,0xffffff);
}
static void training_cell(struct type_hud *h,size_t bpp,const struct training_stats *s,
                          unsigned stat,unsigned left,unsigned top,const char *label,unsigned options)
{
    stats_text(h,bpp,left+2,top,label,0xc0d0e0);
    if(s->generation==3 && s->nature_known && (options&HUD_DETAILS))
        nature_arrow(h,bpp,gen3_nature_effect(s->nature,stat),left+20,top);
    char value[6];
    snprintf(value,sizeof(value),"%u",(unsigned)s->dv[stat]);
    stats_text(h,bpp,left+39-(unsigned)strlen(value)*6,top,value,0xffffff);
    if(options&HUD_COMPACT) return;
    uint16_t number=s->ev[stat];bool known=true;
    if(options&HUD_GAINS){known=s->gain_known;number=s->gain[stat];}
    else if(options&HUD_BONUS)known=training_bonus(s,stat,&number);
    if(known)snprintf(value,sizeof(value),"%u",(unsigned)number);
    else strcpy(value,"NA");
    stats_text(h,bpp,left+79-(unsigned)strlen(value)*6,top,value,0xffffff);
}
static void training_table(struct type_hud *h,size_t bpp,const struct training_stats *s,unsigned options)
{
    const char *labels[]={"HP","ATK","DEF","SPA","SPD","SPE"};
    const char *heading=options&HUD_GAINS?"GAIN":options&HUD_BONUS?"BON":s->generation==3?"EV":"EXP";
    if(s->generation==3) {
        /* Two three-row columns replace the EXP pane below stats, preserving portrait,
         * held item, level and all original stat numbers. */
        for(unsigned y=112;y<160;y++) for(unsigned x=80;x<240;x++)
            pixel(h->output,h->pitch,bpp,x,y,h->format,0x18202c);
        for(unsigned col=0;col<2;col++) {
            unsigned left=80+col*80;
            stats_text(h,bpp,left+27,113,"IV",0x90c4ff);if(!(options&HUD_COMPACT)) stats_text(h,bpp,left+49,113,heading,0x90c4ff);
            for(unsigned row=0;row<3;row++) {
                unsigned i=col*3+row;training_cell(h,bpp,s,i,left,121+row*10,labels[i],options);
            }
        }

        unsigned used,left;char text[24];
        if(options&HUD_GAINS)snprintf(text,sizeof(text),"%s",s->gain_known?"LAST BATTLE":"NO BASELINE");
        else if(training_budget(s,&used,&left))snprintf(text,sizeof(text),"EV%u R%u",used,left);
        else strcpy(text,"EV NA");
        if(!(options&HUD_COMPACT)) stats_text(h,bpp,82,152,text,0x90c4ff);

        return;
    }
    unsigned left=s->generation==1 ? 80 : 0,top=s->generation==1 ? 72 : 64;
    for(unsigned y=top;y<144;y++) for(unsigned x=left;x<left+80;x++)
        pixel(h->output,h->pitch,bpp,x,y,h->format,0x18202c);
    stats_text(h,bpp,left+27,top+2,"DV",0x90c4ff);if(!(options&HUD_COMPACT)) stats_text(h,bpp,left+49,top+2,heading,0x90c4ff);
    const unsigned kanto_order[]={0,1,2,5,3};
    for(unsigned row=0;row<(s->generation==1 ? 5u : 6u);row++) {
        unsigned i=s->generation==1 ? kanto_order[row] : row;
        training_cell(h,bpp,s,i,left,top+(s->generation==2 ? 11u : 12u)+row*10,s->generation==1 && i==3 ? "SPC" : labels[i],options);
    }
    if(s->generation==1 && !(options&HUD_COMPACT)) stats_text(h,bpp,left+2,137,
        options&HUD_GAINS?(s->gain_known?"LAST BATTLE":"NO BASELINE"):options&HUD_BONUS?"STAT BONUS":"STAT EXP",0x90c4ff);
}
static bool reserve(uint8_t **buffer, size_t *capacity, size_t bytes)
{
    if(bytes<=*capacity) return true;
    uint8_t *next=realloc(*buffer,bytes);
    if(!next) return false;
    *buffer=next; *capacity=bytes; return true;
}
const void *type_hud_draw_options(struct type_hud *h, const struct battle_state *state,
    const void *frame, unsigned width, unsigned height, size_t pitch,
    enum retro_pixel_format format, unsigned options)
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
        if(h->has_presented && battle_state_equal(&h->presented,state) && h->presented_options==options) return NULL;
    } else {
        if(!reserve(&h->clean,&h->clean_capacity,bytes)) {
            h->has_frame=false; h->has_presented=false; return frame;
        }
        for(unsigned y=0;y<height;y++)
            memcpy(h->clean+(size_t)y*pitch,(const uint8_t *)frame+(size_t)y*pitch,row_bytes);
        h->width=width; h->height=height; h->pitch=pitch; h->format=format; h->has_frame=true;
    }
    const void *result=frame?frame:h->clean;
    bool active=(options&HUD_DEFAULT) && fits(state,width,height);
    if(active && reserve(&h->output,&h->output_capacity,bytes)) {
        for(unsigned y=0;y<height;y++)
            memcpy(h->output+(size_t)y*pitch,h->clean+(size_t)y*pitch,row_bytes);
        if(state->training.visible) {
            const struct training_stats *s=&state->training;
            if(options&HUD_TRAINING) training_table(h,bpp,s,options);
            if((options&HUD_DETAILS) && s->generation==3) party_details_line(h,bpp,s);
            if((options&HUD_POWER) && (s->generation==2 || s->generation==3)) {
                unsigned left=s->generation==3 ? 160 : 0,top=s->generation==3 ? 148 : 132;
                for(unsigned y=top;y<top+12;y++) for(unsigned x=left;x<left+80;x++)
                    pixel(h->output,h->pitch,bpp,x,y,h->format,0x18202c);
                hidden_power_row(h,bpp,s,left,top);
            }
        }
        else {
            if((options&HUD_TYPES) && !state->ambiguous_target) badge(h,bpp,state->type1,2);
            if((options&HUD_TYPES) && !state->ambiguous_target && state->type2!=TYPE_NONE && state->type2!=state->type1) badge(h,bpp,state->type2,16);
            if(options&HUD_MOVES) move_hints(h,bpp,state);
        }
        result=h->output;
    } else if(active) {
        /* Allocation failure passes a clean frame; retry if a duplicate arrives. */
        h->has_presented=false;
        return result;
    }
    h->presented=*state; h->presented_options=options; h->has_presented=true;
    return result;
}
void type_hud_clear(struct type_hud *h)
{
    free(h->clean); free(h->output); memset(h,0,sizeof(*h));
}

const void *type_hud_draw(struct type_hud *h,const struct battle_state *s,
    const void *frame,unsigned width,unsigned height,size_t pitch,enum retro_pixel_format format)
{
    return type_hud_draw_options(h,s,frame,width,height,pitch,format,HUD_DEFAULT);
}

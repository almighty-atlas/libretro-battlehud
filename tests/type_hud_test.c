#ifdef NDEBUG
#undef NDEBUG
#endif
#include "type_hud.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static size_t bpp(enum retro_pixel_format f) { return f==RETRO_PIXEL_FORMAT_XRGB8888?4:2; }
static uint32_t get(const void *p,size_t pitch,size_t size,unsigned x,unsigned y)
{
    uint32_t v=0; memcpy(&v,(const uint8_t *)p+y*pitch+x*size,size); return v;
}
static uint32_t packed(enum retro_pixel_format f,uint32_t rgb)
{
    if(f==RETRO_PIXEL_FORMAT_XRGB8888) return rgb;
    unsigned r=rgb>>16&255,g=rgb>>8&255,b=rgb&255;
    return f==RETRO_PIXEL_FORMAT_RGB565 ? (r>>3)<<11|(g>>2)<<5|(b>>3) :
        (r>>3)<<10|(g>>3)<<5|(b>>3);
}
static struct battle_state model(enum pokemon_type t1,enum pokemon_type t2)
{
    struct battle_state s={0}; s.status=BATTLE_ACTIVE; s.mode=2; s.species=16;
    s.type1=t1; s.type2=t2; s.main_menu=true; return s;
}
static void check(enum retro_pixel_format f)
{
    const unsigned w=160,h=144; size_t size=bpp(f),pitch=w*size+7;
    size_t bytes=pitch*(h-1)+w*size;
    /* Exact allocation omits the last row's trailing padding. */
    uint8_t *input=malloc(bytes), *original=malloc(bytes); assert(input && original);
    memset(input,0x5a,bytes); memcpy(original,input,bytes);
    struct type_hud hud={0};
    struct battle_state s=model(TYPE_NORMAL,TYPE_FLYING), hidden={0};
    const void *out=type_hud_draw(&hud,&s,input,w,h,pitch,f);
    assert(out!=input);
    assert(!memcmp(input,original,bytes));
    /* Both NORMAL and FLYING are 6 glyphs: 43px wide, 13px high. */
    unsigned left=115;
    for(unsigned y=0;y<h;y++) for(unsigned x=0;x<w;x++) {
        bool badge=(x>=left && x<158 && ((y>=2 && y<15)||(y>=17 && y<30)));
        if(!badge) assert(!memcmp((const uint8_t *)out+y*pitch+x*size,input+y*pitch+x*size,size));
    }
    assert(get(out,pitch,size,left,2)==packed(f,0x101010));
    assert(get(out,pitch,size,left+1,3)==packed(f,0x606060));
    assert(get(out,pitch,size,left+4,5)==packed(f,0xffffff)); /* N first pixel */
    assert(get(out,pitch,size,left+1,18)==packed(f,0x5c5189));
    uint8_t *clean=hud.clean,*output=hud.output;
    assert(type_hud_draw(&hud,&s,NULL,w,h,pitch,f)==NULL); /* Identical duplicate. */
    assert(type_hud_draw(&hud,&s,input,w,h,pitch,f)==output);
    assert(hud.clean==clean && hud.output==output); /* Allocation reuse. */
    /* Enemy changes on a duplicate: restore clean pixels, remove old second row. */
    s=model(TYPE_GRASS,TYPE_NONE); s.species=152;
    out=type_hud_draw(&hud,&s,NULL,w,h,pitch,f);
    assert(out==output);
    for(unsigned y=17;y<30;y++) for(unsigned x=115;x<158;x++)
        assert(get(out,pitch,size,x,y)==get(input,pitch,size,x,y));
    unsigned grass_left=121;
    assert(get(out,pitch,size,grass_left+1,3)==packed(f,0x427033));
    /* Enter a submenu on a duplicate: the enemy stays valid, badges vanish. */
    s.main_menu=false;
    out=type_hud_draw(&hud,&s,NULL,w,h,pitch,f); assert(out==clean);
    for(unsigned y=0;y<h;y++) assert(!memcmp((const uint8_t *)out+y*pitch,input+y*pitch,w*size));
    assert(type_hud_draw(&hud,&s,NULL,w,h,pitch,f)==NULL);
    s.main_menu=true;
    out=type_hud_draw(&hud,&s,NULL,w,h,pitch,f); assert(out==output);
    assert(get(out,pitch,size,grass_left+1,3)==packed(f,0x427033));
    /* End on a duplicate must send an unmarked frame, not retain old badges. */
    out=type_hud_draw(&hud,&hidden,NULL,w,h,pitch,f); assert(out==clean);
    for(unsigned y=0;y<h;y++) assert(!memcmp((const uint8_t *)out+y*pitch,input+y*pitch,w*size));
    assert(type_hud_draw(&hud,&hidden,NULL,w,h,pitch,f)==NULL);
    assert(type_hud_draw(&hud,&hidden,input,w,h,pitch,f)==input);
    for(enum pokemon_type t=TYPE_NORMAL;t<=TYPE_FAIRY;t++) {
        s=model(t,TYPE_NONE); out=type_hud_draw(&hud,&s,input,w,h,pitch,f);
        assert(out==output); /* Every supported label fits and remains within frame. */
        assert(!memcmp(input,original,bytes));
    }
    s=model(TYPE_NONE,TYPE_NONE);
    assert(type_hud_draw(&hud,&s,input,w,h,pitch,f)==input);
    s=model(TYPE_NORMAL,(enum pokemon_type)99);
    assert(type_hud_draw(&hud,&s,input,w,h,pitch,f)==input);
    s=model(TYPE_NORMAL,TYPE_NORMAL);
    out=type_hud_draw(&hud,&s,input,w,h,pitch,f);
    assert(get(out,pitch,size,116,18)==get(input,pitch,size,116,18));
    s.status=BATTLE_INVALID;
    assert(type_hud_draw(&hud,&s,input,w,h,pitch,f)==input);
    s=model(TYPE_NORMAL,TYPE_FLYING);
    assert(type_hud_draw(&hud,&s,RETRO_HW_FRAME_BUFFER_VALID,w,h,pitch,f)==RETRO_HW_FRAME_BUFFER_VALID);
    assert(type_hud_draw(&hud,&s,NULL,w,h,pitch,f)==NULL);
    assert(type_hud_draw(&hud,&s,input,w,h,pitch,(enum retro_pixel_format)99)==input);
    assert(type_hud_draw(&hud,&s,input,w,h,w*size-1,f)==input);
    assert(type_hud_draw(&hud,&s,input,w,h,SIZE_MAX,f)==input);
    assert(type_hud_draw(&hud,&s,input,0,h,pitch,f)==input);
    assert(type_hud_draw(&hud,&s,input,w,5000,pitch,f)==input);
    /* Valid but too-small frames remain readable without partial text. */
    assert(type_hud_draw(&hud,&s,input,8,8,8*size,f)==input);
    assert(type_hud_draw(&hud,&s,NULL,w,h,pitch,f)==NULL); /* Layout changed. */
    type_hud_clear(&hud);
    assert(!hud.clean && !hud.output && !hud.has_frame);
    assert(type_hud_draw(&hud,&s,NULL,w,h,pitch,f)==NULL);
    type_hud_clear(&hud); free(input); free(original);
}
static void preview(const char *path)
{
    uint32_t frame[160*144];
    for(unsigned y=0;y<144;y++) for(unsigned x=0;x<160;x++)
        frame[y*160+x]=((x/8+y/8)%2)?0xc0d0b0:0xd0e0c0;
    struct type_hud hud={0}; struct battle_state s=model(TYPE_NORMAL,TYPE_FLYING);
    const uint32_t *out=type_hud_draw(&hud,&s,frame,160,144,640,RETRO_PIXEL_FORMAT_XRGB8888);
    FILE *f=fopen(path,"wb"); assert(f);
    fprintf(f,"P6\n160 144\n255\n");
    for(unsigned i=0;i<160*144;i++) {
        unsigned char rgb[]={out[i]>>16,out[i]>>8,out[i]};
        assert(fwrite(rgb,1,3,f)==3);
    }
    assert(fclose(f)==0); type_hud_clear(&hud);
}
int main(int argc,char **argv)
{
    check(RETRO_PIXEL_FORMAT_0RGB1555); check(RETRO_PIXEL_FORMAT_RGB565); check(RETRO_PIXEL_FORMAT_XRGB8888);
    if(argc==2) preview(argv[1]);
    puts("M4 badge pixels, formats, source immutability, duplicate updates and cleanup passed");
    return 0;
}

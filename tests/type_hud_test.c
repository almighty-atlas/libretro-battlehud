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
    /* Two original 8x8 silhouettes in 12x12 tiles. */
    unsigned left=146;
    for(unsigned y=0;y<h;y++) for(unsigned x=0;x<w;x++) {
        bool badge=(x>=left && x<158 && ((y>=2 && y<14)||(y>=16 && y<28)));
        if(!badge) assert(!memcmp((const uint8_t *)out+y*pitch+x*size,input+y*pitch+x*size,size));
    }
    assert(get(out,pitch,size,left,2)==packed(f,0x101010));
    assert(get(out,pitch,size,left+1,3)==packed(f,0x606060));
    assert(get(out,pitch,size,left+5,4)==packed(f,0xffffff)); /* Normal diamond top */
    assert(get(out,pitch,size,left+1,17)==packed(f,0x5c5189));
    uint8_t *clean=hud.clean,*output=hud.output;
    assert(type_hud_draw(&hud,&s,NULL,w,h,pitch,f)==NULL); /* Identical duplicate. */
    assert(type_hud_draw(&hud,&s,input,w,h,pitch,f)==output);
    assert(hud.clean==clean && hud.output==output); /* Allocation reuse. */
    /* Enemy changes on a duplicate: restore clean pixels, remove old second row. */
    s=model(TYPE_GRASS,TYPE_NONE); s.species=152;
    out=type_hud_draw(&hud,&s,NULL,w,h,pitch,f);
    assert(out==output);
    for(unsigned y=16;y<28;y++) for(unsigned x=146;x<158;x++)
        assert(get(out,pitch,size,x,y)==get(input,pitch,size,x,y));
    unsigned grass_left=146;
    assert(get(out,pitch,size,grass_left+1,3)==packed(f,0x427033));
    /* Main menu -> FIGHT retains badges on the same clean frame. */
    s.main_menu=false; s.fight_menu=true;
    out=type_hud_draw(&hud,&s,NULL,w,h,pitch,f); assert(out==output);
    assert(get(out,pitch,size,grass_left+1,3)==packed(f,0x427033));
    s.moves[0]=52; s.effectiveness[0]=MOVE_SUPER;
    s.moves[1]=45; s.effectiveness[1]=MOVE_STATUS;
    out=type_hud_draw(&hud,&s,NULL,w,h,pitch,f); assert(out==output);
    assert(get(out,pitch,size,147,104)==packed(f,0xffffff));
    assert(get(out,pitch,size,144,104)==packed(f,0x206038));
    assert(get(out,pitch,size,145,115)==packed(f,0xffffff)); /* status dash */
    for(unsigned y=0;y<h;y++) for(unsigned x=0;x<w;x++) {
        bool icon=x>=146 && x<158 && y>=2 && y<14;
        bool hint=x>=144 && x<151 && ((y>=104 && y<111)||(y>=112 && y<119));
        if(!icon && !hint) assert(get(out,pitch,size,x,y)==get(input,pitch,size,x,y));
    }
    s.effectiveness[0]=MOVE_IMMUNE; /* Redraw hints on duplicate even with same enemy. */
    out=type_hud_draw(&hud,&s,NULL,w,h,pitch,f); assert(out==output);
    assert(get(out,pitch,size,144,104)==packed(f,0x902c30));
    s.moves[1]=0;
    out=type_hud_draw(&hud,&s,NULL,w,h,pitch,f); assert(out==output);
    assert(get(out,pitch,size,145,115)==get(input,pitch,size,145,115));
    const uint32_t hint_colors[]={0x665000,0x206038,0x825016,0x405060,
                                 0x902c30,0x505050,0x505050};
    for(unsigned k=MOVE_UNKNOWN;k<=MOVE_UNUSABLE;k++) {
        s.moves[3]=237; s.effectiveness[3]=(uint8_t)k;
        out=type_hud_draw(&hud,&s,NULL,w,h,pitch,f); assert(out==output);
        assert(get(out,pitch,size,144,128)==packed(f,hint_colors[k]));
        assert(!memcmp(input,original,bytes));
    }
    /* Move annotations require exact GB geometry; type icons can still fit. */
    s.moves[3]=0;
    out=type_hud_draw(&hud,&s,input,w,h-1,pitch,f); assert(out==output);
    assert(get(out,pitch,size,144,104)==get(input,pitch,size,144,104));
    out=type_hud_draw(&hud,&s,input,w,h,pitch,f); assert(out==output);
    s.fight_menu=false;
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
    assert(get(out,pitch,size,147,17)==get(input,pitch,size,147,17));
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
    /* Training pane overlays only x=0..79, y=64..143, even outside battle. */
    memset(&s,0,sizeof(s)); s.training.visible=true; s.training.generation=2; s.training.species=155;
    for(unsigned i=0;i<6;i++) { s.training.dv[i]=15; s.training.ev[i]=65535; }
    out=type_hud_draw(&hud,&s,input,w,h,pitch,f); assert(out==hud.output);
    for(unsigned y=0;y<h;y++) for(unsigned x=0;x<w;x++)
        if(x>=80 || y<64) assert(get(out,pitch,size,x,y)==get(input,pitch,size,x,y));
    assert(get(out,pitch,size,0,64)==packed(f,0x18202c));
    assert(get(out,pitch,size,28,66)==packed(f,0x90c4ff)); /* D header */
    assert(get(out,pitch,size,29,75)==packed(f,0xffffff)); /* 1 in HP DV 15 */
    assert(get(out,pitch,size,50,75)==packed(f,0xffffff)); /* first 6 of 65535 */
    assert(!memcmp(input,original,bytes));
    assert(type_hud_draw(&hud,&s,NULL,w,h,pitch,f)==NULL);
    s.training.ev[0]=0; s.training.slot=1;
    assert(type_hud_draw(&hud,&s,NULL,w,h,pitch,f)==hud.output);
    out=type_hud_draw(&hud,&hidden,NULL,w,h,pitch,f); assert(out==hud.clean);
    for(unsigned y=0;y<h;y++) assert(!memcmp((const uint8_t *)out+y*pitch,input+y*pitch,w*size));
    s.training.visible=true;
    assert(type_hud_draw(&hud,&s,input,w,h-1,pitch,f)==input);
    type_hud_clear(&hud); free(input); free(original);
}
static void multigen_pane(enum retro_pixel_format f,unsigned generation)
{
    unsigned w=generation==3 ? 240 : 160,h=generation==3 ? 160 : 144;
    size_t bpp=f==RETRO_PIXEL_FORMAT_XRGB8888 ? 4 : 2,pitch=w*bpp+8,bytes=pitch*h;
    uint8_t *frame=malloc(bytes),*original=malloc(bytes);assert(frame && original);
    memset(frame,0x5a,bytes);memcpy(original,frame,bytes);
    struct type_hud hud={0};struct battle_state s={0},hidden={0};
    s.training.visible=true;s.training.generation=(uint8_t)generation;s.training.species=277;
    for(unsigned i=0;i<6;i++){s.training.dv[i]=generation==3 ? 31 : 15;s.training.ev[i]=generation==3 ? 85 : 65535;}
    const void *out=type_hud_draw(&hud,&s,frame,w,h,pitch,f);assert(out==hud.output);
    unsigned changed=0;
    for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++) {
        bool pane=generation==3 ? x>=80 && y>=112 : x>=80 && y>=72;
        if(!pane)assert(get(out,pitch,bpp,x,y)==get(frame,pitch,bpp,x,y));
        else if(get(out,pitch,bpp,x,y)!=get(frame,pitch,bpp,x,y))changed++;
    }
    assert(changed && !memcmp(frame,original,bytes));
    assert(get(out,pitch,bpp,80,generation==3 ? 112 : 72)==packed(f,0x18202c));
    assert(type_hud_draw(&hud,&s,NULL,w,h,pitch,f)==NULL);
    s.training.slot=1;s.training.dv[0]=0;
    assert(type_hud_draw(&hud,&s,NULL,w,h,pitch,f)==hud.output);
    out=type_hud_draw(&hud,&hidden,NULL,w,h,pitch,f);assert(out==hud.clean);
    for(unsigned y=0;y<h;y++)assert(!memcmp((const uint8_t *)out+y*pitch,frame+y*pitch,w*bpp));
    assert(type_hud_draw(&hud,&s,frame,w-1,h,pitch,f)==frame);
    type_hud_clear(&hud);free(frame);free(original);
}
static void hidden_power_pixels(enum retro_pixel_format f,unsigned generation)
{
    unsigned w=generation==3 ? 240 : 160,h=generation==3 ? 160 : 144;
    unsigned left=generation==3 ? 160 : 0,top=generation==3 ? 148 : 132;
    size_t bpp=f==RETRO_PIXEL_FORMAT_XRGB8888 ? 4 : 2,pitch=w*bpp;
    uint8_t *frame=calloc(h,pitch);assert(frame);struct type_hud hud={0};struct battle_state s={0},hidden={0};
    s.training.visible=true;s.training.generation=(uint8_t)generation;
    for(unsigned i=0;i<6;i++)s.training.dv[i]=generation==3 ? 31 : 15;
    const void *out=type_hud_draw(&hud,&s,frame,w,h,pitch,f);assert(out==hud.output);
    assert(get(out,pitch,bpp,left+28,top)==packed(f,0x101010)); /* tile border */
    assert(get(out,pitch,bpp,left+29,top+1)==packed(f,0x493e39)); /* Dark background */
    assert(get(out,pitch,bpp,left+32,top+2)==packed(f,0xffffff)); /* Dark icon */
    assert(get(out,pitch,bpp,left+52,top+4)==packed(f,0xffffff)); /* P in P 70 */
    assert(get(out,pitch,bpp,left+68,top+4)==packed(f,0xffffff)); /* 7 in P 70 */
    assert(type_hud_draw(&hud,&s,NULL,w,h,pitch,f)==NULL);
    s.training.dv[generation==3 ? 0 : 1]--;
    out=type_hud_draw(&hud,&s,NULL,w,h,pitch,f);assert(out==hud.output);
    assert(get(out,pitch,bpp,left+29,top+1)==packed(f,generation==3 ? 0x514096 : 0x866600));
    /* Base power redraw without changing the type: 70 -> 30 (G3), 70 -> 32 (G2). */
    for(unsigned i=0;i<6;i++)s.training.dv[i]&=generation==3 ? 1 : 3;
    out=type_hud_draw(&hud,&s,NULL,w,h,pitch,f);assert(out==hud.output);
    assert(get(out,pitch,bpp,left+29,top+1)==packed(f,generation==3 ? 0x514096 : 0x866600));
    assert(get(out,pitch,bpp,left+68,top+4)==packed(f,0x18202c)); /* 3 lacks the last pixel in its top row */
    s.training.dv[1]=255;out=type_hud_draw(&hud,&s,NULL,w,h,pitch,f);assert(out==hud.output);
    assert(get(out,pitch,bpp,left+28,top)==packed(f,0x18202c)); /* Invalid HP value -> no partial row */
    out=type_hud_draw(&hud,&hidden,NULL,w,h,pitch,f);assert(out==hud.clean);
    for(unsigned y=0;y<h;y++)assert(!memcmp((const uint8_t *)out+y*pitch,frame+y*pitch,pitch));
    type_hud_clear(&hud);free(frame);
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
    for(unsigned f=0;f<3;f++){multigen_pane((enum retro_pixel_format)f,1);multigen_pane((enum retro_pixel_format)f,3);}
    for(unsigned f=0;f<3;f++){hidden_power_pixels((enum retro_pixel_format)f,2);hidden_power_pixels((enum retro_pixel_format)f,3);}
    if(argc==2) preview(argv[1]);
    puts("M4 badge pixels, formats, source immutability, duplicate updates and cleanup passed");
    return 0;
}

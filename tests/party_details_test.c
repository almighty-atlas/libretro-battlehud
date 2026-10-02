#ifdef NDEBUG
#undef NDEBUG
#endif
#include "party_details.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t rom[0x340000];static size_t missing;
static bool read_rom(void *ctx,size_t address,void *out,size_t n)
{
    (void)ctx;if(address<0x08000000 || address>=0x08340000 || n>0x08340000-address || (missing && missing>=address && missing-address<n))return false;
    memcpy(out,rom+address-0x08000000,n);return true;
}
static uint8_t *at(size_t address){assert(address>=0x08000000 && address<0x08340000);return rom+address-0x08000000;}
static void name_entry(const struct game_profile *p,uint8_t id,const char *s)
{
    uint8_t *b=at(p->ability_names+id*13);memset(b,0xff,13);
    for(unsigned i=0;s[i] && i<12;i++)b[i]=s[i]==' ' ? 0 : (uint8_t)(s[i]-'A'+0xbb);
}
int main(void)
{
    const char *names[]={"HARDY","LONELY","BRAVE","ADAMANT","NAUGHTY","BOLD","DOCILE","RELAXED","IMPISH","LAX","TIMID","HASTY","SERIOUS","JOLLY","NAIVE","MODEST","MILD","QUIET","BASHFUL","RASH","CALM","GENTLE","SASSY","CAREFUL","QUIRKY"};
    /* Literal original table: columns ATK/DEF/SPE/SPA/SPD, independent of helper's quotient mapping. */
    const int expected[25][5]={
        {0,0,0,0,0},{1,-1,0,0,0},{1,0,-1,0,0},{1,0,0,-1,0},{1,0,0,0,-1},
        {-1,1,0,0,0},{0,0,0,0,0},{0,1,-1,0,0},{0,1,0,-1,0},{0,1,0,0,-1},
        {-1,0,1,0,0},{0,-1,1,0,0},{0,0,0,0,0},{0,0,1,-1,0},{0,0,1,0,-1},
        {-1,0,0,1,0},{0,-1,0,1,0},{0,0,-1,1,0},{0,0,0,0,0},{0,0,0,1,-1},
        {-1,0,0,0,1},{0,-1,0,0,1},{0,0,-1,0,1},{0,0,0,-1,1},{0,0,0,0,0}
    };
    const unsigned game_to_display[]={1,2,5,3,4};
    for(unsigned n=0;n<25;n++) {
        assert(!strcmp(gen3_nature_name((uint8_t)n),names[n]) && !gen3_nature_effect((uint8_t)n,0));
        unsigned up=0,down=0;
        for(unsigned stat=0;stat<5;stat++){int x=gen3_nature_effect((uint8_t)n,game_to_display[stat]);assert(x==expected[n][stat]);up+=x==1;down+=x==-1;}
        assert((up==1 && down==1) || (!up && !down));
    }
    assert(!gen3_nature_name(25) && !gen3_nature_name(255));assert(!gen3_nature_effect(255,1) && !gen3_nature_effect(3,6));
    const struct game_profile *p=game_profile_find("f3ae088181bf583e55daf962a92bb46f4f1d07b7");assert(p);
    *at(p->species_info+19*28+22)=50;*at(p->species_info+19*28+23)=62;
    name_entry(p,50,"RUN AWAY");name_entry(p,62,"GUTS");name_entry(p,31,"LIGHTNINGROD");
    uint8_t ability=99;char name[13];memset(name,0x5a,13);
    assert(gen3_ability_read(p,read_rom,NULL,19,0,&ability,name) && ability==50 && !strcmp(name,"RUN AWAY"));
    assert(gen3_ability_read(p,read_rom,NULL,19,1,&ability,name) && ability==62 && !strcmp(name,"GUTS"));
    *at(p->species_info+277*28+22)=65;name_entry(p,65,"OVERGROW");
    assert(gen3_ability_read(p,read_rom,NULL,277,0,&ability,name) && ability==65 && !strcmp(name,"OVERGROW"));
    assert(gen3_ability_read(p,read_rom,NULL,277,1,&ability,name) && ability==0 && !strcmp(name,"NONE")); /* No invented fallback to slot 0. */
    *at(p->species_info+19*28+22)=31;
    assert(gen3_ability_read(p,read_rom,NULL,19,0,&ability,name) && !strcmp(name,"LIGHTNINGROD") && !name[12]);
    for(unsigned i=0;i<12;i++)at(p->ability_names+31*13)[i]=(uint8_t)(at(p->ability_names+31*13)[i]+26);
    assert(gen3_ability_read(p,read_rom,NULL,19,0,&ability,name) && !strcmp(name,"LIGHTNINGROD")); /* Lowercase ROM letters normalize. */
    for(unsigned id=1;id<78;id++) {
        *at(p->species_info+19*28+22)=(uint8_t)id;name_entry(p,(uint8_t)id,"VALID NAME");
        assert(gen3_ability_read(p,read_rom,NULL,19,0,&ability,name) && ability==id && !strcmp(name,"VALID NAME"));
    }
    *at(p->species_info+19*28+22)=31;
    for(unsigned test=0;test<4;test++) {
        name_entry(p,31,"LIGHTNINGROD");uint8_t *entry=at(p->ability_names+31*13);
        if(test==0)entry[12]=0xbb; /* No EOS */
        if(test==1)entry[0]=0xff; /* Empty */
        if(test==2)entry[0]=0xfc; /* Control code */
        if(test==3){memset(entry,0,12);entry[12]=0xff;} /* All spaces */
        assert(!gen3_ability_read(p,read_rom,NULL,19,0,&ability,name));
    }
    name_entry(p,31,"LIGHTNINGROD");
    const size_t reads[]={p->species_info+19*28+22,p->ability_names+31*13};
    for(unsigned i=0;i<2;i++){missing=reads[i];assert(!gen3_ability_read(p,read_rom,NULL,19,0,&ability,name));missing=0;}
    *at(p->species_info+19*28+22)=78;assert(!gen3_ability_read(p,read_rom,NULL,19,0,&ability,name));
    assert(!gen3_ability_read(NULL,read_rom,NULL,19,0,&ability,name));assert(!gen3_ability_read(p,NULL,NULL,19,0,&ability,name));
    assert(!gen3_ability_read(p,read_rom,NULL,0,0,&ability,name));assert(!gen3_ability_read(p,read_rom,NULL,252,0,&ability,name));
    assert(!gen3_ability_read(p,read_rom,NULL,412,0,&ability,name));assert(!gen3_ability_read(p,read_rom,NULL,19,2,&ability,name));
    assert(!gen3_ability_read(p,read_rom,NULL,19,0,NULL,name));assert(!gen3_ability_read(p,read_rom,NULL,19,0,&ability,NULL));
    const struct game_profile *gb=game_profile_find("ea9bcae617fdf159b045185467ae58b2e4a48b9a");assert(!gen3_ability_read(gb,read_rom,NULL,19,0,&ability,name));
    puts("All 25 natures, HP neutrality, both stored ability slots, all IDs, maximum name length, decoding and missing/invalid ROM data passed");
}

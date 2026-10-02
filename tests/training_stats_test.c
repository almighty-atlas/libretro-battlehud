#ifdef NDEBUG
#undef NDEBUG
#endif
#include "training_stats.h"
#include "party_details.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static uint8_t ram[0x2000],rom[0x80000];
static size_t missing;
static bool read_ram(void *ctx,size_t address,void *out,size_t bytes)
{
    (void)ctx;
    if(address==missing)return false;
    if(address>=0x10000000 && address<0x10080000 && bytes<=0x10080000-address){memcpy(out,rom+address-0x10000000,bytes);return true;}
    if(address==missing || address<0xc000 || address>=0xe000 || bytes>0xe000-address) return false;
    memcpy(out,ram+address-0xc000,bytes); return true;
}
static void put(unsigned address,uint8_t value) { ram[address-0xc000]=value; }
static void fixture(unsigned slot,uint8_t dvs1,uint8_t dvs2)
{
    memset(ram,0,sizeof(ram));
    const uint8_t cyndaquil[]={155,39,52,43,65,60,50};memcpy(rom+0x51424+154*32,cyndaquil,7);
    put(0xcf64,3); put(0xcf63,6); put(0xcf5f,0);
    put(0xd109,(uint8_t)slot); put(0xdcd7,6); put(0xd108,155);
    put(0xc515,0x3a); put(0xc516,0x3b);
    const uint8_t attack[]={0x80,0x93,0x93,0x80,0x82,0x8a};
    const uint8_t defense[]={0x83,0x84,0x85,0x84,0x8d,0x92,0x84};
    const uint8_t sat[]={0x92,0x8f,0x82,0x8b,0xe8,0x80,0x93,0x8a};
    const uint8_t sdf[]={0x92,0x8f,0x82,0x8b,0xe8,0x83,0x84,0x85};
    const uint8_t speed[]={0x92,0x8f,0x84,0x84,0x83};
    memcpy(ram+0x54b,attack,6); memcpy(ram+0x573,defense,7);
    memcpy(ram+0x59b,sat,8); memcpy(ram+0x5c3,sdf,8); memcpy(ram+0x5eb,speed,5);
    uint8_t *temp=ram+0x110e; temp[0]=155; temp[31]=10;
    temp[21]=dvs1; temp[22]=dvs2;
    const uint16_t evs[5]={0,1,256,65535,0x1234};
    for(unsigned i=0;i<5;i++) {temp[11+i*2]=(uint8_t)(evs[i]>>8);temp[12+i*2]=(uint8_t)evs[i];}
    memcpy(ram+0x1cdf+slot*48,temp,48);
}
int main(void)
{
    const struct game_profile *p=game_profile_find("f2f52230b536214ef7c9924f483392993e226cfb");
    fixture(0,0xa5,0xc3);
    struct training_stats s=training_stats_decode(p,read_ram,NULL);
    const uint8_t expected_dvs[6]={5,10,5,3,3,12};
    const uint16_t expected_evs[6]={0,1,256,0x1234,0x1234,65535};
    const uint8_t base[]={39,52,43,60,50,65};
    assert(s.level==10 && s.bonus_known && s.identity_known && !memcmp(s.base,base,6));
    struct training_stats direct=training_party_mon(p,read_ram,NULL,0);assert(training_stats_equal(&s,&direct));
    assert(s.visible && s.slot==0 && s.species==155);
    assert(!memcmp(s.dv,expected_dvs,sizeof(expected_dvs)));
    assert(!memcmp(s.ev,expected_evs,sizeof(expected_evs)));
    assert(training_stats_equal(&s,&s));
    for(unsigned slot=0;slot<6;slot++) {
        fixture(slot,0xff,0xff); s=training_stats_decode(p,read_ram,NULL);
        assert(s.visible && s.slot==slot);
        for(unsigned i=0;i<6;i++) assert(s.dv[i]==15);
    }
    for(unsigned parity=0;parity<16;parity++) {
        fixture(0,(uint8_t)(((parity>>3)&1)*16+((parity>>2)&1)),
                  (uint8_t)(((parity>>1)&1)*16+(parity&1)));
        s=training_stats_decode(p,read_ram,NULL); assert(s.visible && s.dv[0]==parity);
    }
    const unsigned addresses[]={0xcf64,0xcf63,0xcf5f,0xd109,0xdcd7,0xd108,
                                0xc515,0xc54b,0xc573,0xc59b,0xc5c3,0xc5eb,0xd10e,0xdcdf};
    for(unsigned i=0;i<sizeof(addresses)/sizeof(*addresses);i++) {
        fixture(0,0,0); missing=addresses[i];
        s=training_stats_decode(p,read_ram,NULL); assert(!s.visible);
    }
    missing=0;
    const struct {unsigned address;uint8_t bad;} cases[]={
        {0xcf64,1},{0xcf64,2},{0xcf63,4},{0xcf63,5},{0xcf63,7},{0xcf63,0x86},
        {0xcf5f,1},{0xcf5f,2},{0xcf5f,3},{0xdcd7,0},{0xdcd7,7},{0xd109,6},
        {0xd108,253},{0xc515,0x36},{0xc54b,0},{0xc59f,0},{0xd10e,0},{0xd12d,101},{0xdcf4,0xff}
    };
    for(unsigned i=0;i<sizeof(cases)/sizeof(*cases);i++) {
        fixture(0,0,0); put(cases[i].address,cases[i].bad);
        s=training_stats_decode(p,read_ram,NULL); assert(!s.visible);
    }
    for(unsigned ratio=0;ratio<256;ratio++)for(unsigned value=0;value<256;value++) {
        rom[0x51424+154*32+13]=(uint8_t)ratio;uint8_t sex=99;
        assert(party_gender_read(p,read_ram,NULL,155,value,&sex));
        assert(sex==(ratio==255?2:ratio==0?0:ratio==254?1:value<=ratio));
    }
    fixture(3,0x2a,0xaa);ram[0x110e + 27]=255;ram[0x110e + 28]=0x20;
    memcpy(ram+0x1cdf+3*48,ram+0x110e,48);
    s=training_stats_decode(p,read_ram,NULL);
    assert(s.visible && s.slot==3 && s.extras_known && s.friendship==255 && s.pokerus==0x20 && s.shiny && s.gender_known);
    struct training_stats altered=s;altered.friendship--;assert(!training_stats_equal(&s,&altered));
    altered=s;altered.pokerus=0;assert(!training_stats_equal(&s,&altered));
    altered=s;altered.shiny=false;assert(!training_stats_equal(&s,&altered));
    missing=p->species_info+154*32;s=training_stats_decode(p,read_ram,NULL);
    assert(s.visible && s.extras_known && s.shiny && !s.gender_known);missing=0;
    assert(!training_stats_decode(NULL,read_ram,NULL).visible);
    assert(!training_stats_decode(p,NULL,NULL).visible);
    puts("Party stats page gating, all slots, HP DV parity, shared Special and big-endian stat exp passed");
}

#ifdef NDEBUG
#undef NDEBUG
#endif
#include "training_stats.h"
#include "battle_decoder.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t gb[0x2000],ewram[0x40000],iwram[0x8000];
static size_t missing;
static uint8_t *at(size_t a,size_t n)
{
    if(a>=0xc000 && a<0xe000 && n<=0xe000-a) return gb+a-0xc000;
    if(a>=0x02000000 && a<0x02040000 && n<=0x02040000-a) return ewram+a-0x02000000;
    if(a>=0x03000000 && a<0x03008000 && n<=0x03008000-a) return iwram+a-0x03000000;
    return NULL;
}
static bool read_memory(void *ctx,size_t a,void *out,size_t n)
{ (void)ctx;uint8_t *p=at(a,n);if(!p || (missing && missing>=a && missing-a<n))return false;memcpy(out,p,n);return true; }
static void put32(uint8_t *b,uint32_t v) {for(unsigned i=0;i<4;i++)b[i]=(uint8_t)(v>>(8*i));}
static void kanto_fixture(const struct game_profile *p,unsigned slot,uint8_t a,uint8_t b)
{
    memset(gb,0,sizeof(gb));*at(p->mon_source,1)=0;*at(p->party_index,1)=(uint8_t)slot;*at(p->party_count,1)=6;
    for(unsigned i=0;i<5;i++)memcpy(at(p->stats_labels[i].address,p->stats_labels[i].size),p->stats_labels[i].tiles,p->stats_labels[i].size);
    uint8_t *mon=at(p->temp_mon,44);mon[0]=177;mon[33]=12;mon[27]=a;mon[28]=b;
    const uint16_t exp[]={0,1,256,65535,0x1234};
    for(unsigned i=0;i<5;i++){mon[17+2*i]=(uint8_t)(exp[i]>>8);mon[18+2*i]=(uint8_t)exp[i];}
    memcpy(at(p->party_base+slot*44,44),mon,44);
}
static void kanto_test(const char *hash)
{
    const struct game_profile *p=game_profile_find(hash);assert(p && p->generation==1);
    for(unsigned slot=0;slot<6;slot++) {
        kanto_fixture(p,slot,0xa5,0xc3);uint8_t before[sizeof(gb)];memcpy(before,gb,sizeof(gb));
        struct training_stats s=training_stats_decode(p,read_memory,NULL);
        const uint8_t dv[]={5,10,5,3,3,12};const uint16_t ev[]={0,1,256,0x1234,0x1234,65535};
        assert(s.visible && s.generation==1 && s.slot==slot && s.species==177);
        assert(!memcmp(s.dv,dv,6) && !memcmp(s.ev,ev,sizeof(ev)) && !memcmp(before,gb,sizeof(gb)));
        struct battle_state state=battle_decode(p,read_memory,NULL);assert(state.status==BATTLE_OUTSIDE && state.training.visible);
    }
    for(unsigned parity=0;parity<16;parity++) {
        kanto_fixture(p,0,(uint8_t)(((parity>>3)&1)*16+((parity>>2)&1)),(uint8_t)(((parity>>1)&1)*16+(parity&1)));
        assert(training_stats_decode(p,read_memory,NULL).dv[0]==parity);
    }
    const size_t bad[]={p->mon_source,p->party_index,p->party_count,p->temp_mon,p->temp_mon+33,p->party_base,p->stats_labels[0].address,p->stats_labels[4].address};
    for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++) {
        kanto_fixture(p,0,0xff,0xff);*at(bad[i],1)=0xff;assert(!training_stats_decode(p,read_memory,NULL).visible);
        kanto_fixture(p,0,0,0);missing=bad[i];assert(!training_stats_decode(p,read_memory,NULL).visible);missing=0;
    }
}
static const uint32_t summary=0x02001000;
/* Physical block order independently encoded as names, rather than decoder's index table. */
static const char *const orders[]={"GAEM","GAME","GEAM","GEMA","GMAE","GMEA","AGEM","AGME","AEGM","AEMG","AMGE","AMEG","EGAM","EGMA","EAGM","EAMG","EMGA","EMAG","MGAE","MGEA","MAGE","MAEG","MEGA","MEAG"};
static void emerald_fixture(const struct game_profile *p,unsigned slot,unsigned permutation,unsigned variant)
{
    memset(ewram,0,sizeof(ewram));memset(iwram,0,sizeof(iwram));
    put32(at(p->main_callback,4),p->summary_callback);put32(at(p->summary_pointer,4),summary);
    put32(at(p->tasks+3*40,4),p->input_task);*at(p->tasks+3*40+4,1)=1;
    put32(at(summary,4),p->gba_party_base);*at(p->gba_party_count,1)=6;
    uint8_t *screen=at(summary+0x40bc,5);screen[0]=0;screen[1]=0;screen[2]=(uint8_t)slot;screen[3]=5;screen[4]=1;
    uint8_t *mon=at(summary+12,100);uint32_t pid=24*12345+permutation,key=pid^0xdeadbeefu;
    put32(mon,pid);put32(mon+4,0xdeadbeefu);mon[19]=2;mon[84]=12;
    uint8_t clear[48]={0};uint32_t iv=31u|(1u<<5)|(17u<<10)|(2u<<15)|(29u<<20)|(3u<<25);
    for(unsigned block=0;block<4;block++) {
        uint8_t *b=clear+block*12;
        if(orders[permutation][block]=='G') {b[0]=277&255;b[1]=277>>8;}
        if(orders[permutation][block]=='E') {const uint8_t ev[]={0,1,252,4,128,125};memcpy(b,ev,6);if(variant){memset(b,0,6);b[0]=b[1]=255;}}
        if(orders[permutation][block]=='M')put32(b+4,iv | (variant==3 ? 1u<<30 : 0));
        if(variant==2 && orders[permutation][block]=='E') b[2]=1;
        if(variant==4 && orders[permutation][block]=='G') b[0]=b[1]=0;
        if(variant==5 && orders[permutation][block]=='G') {b[0]=252;b[1]=0;}
    }
    unsigned checksum=0;for(unsigned i=0;i<48;i+=2)checksum+=clear[i]+((unsigned)clear[i+1]<<8);
    mon[28]=(uint8_t)checksum;mon[29]=(uint8_t)(checksum>>8);
    for(unsigned i=0;i<48;i++)mon[32+i]=clear[i]^(uint8_t)(key>>(8*(i%4)));
    memcpy(at(p->gba_party_base+slot*100,100),mon,100);
}
static void emerald_test(void)
{
    const struct game_profile *p=game_profile_find("f3ae088181bf583e55daf962a92bb46f4f1d07b7");assert(p && p->generation==3 && !strcmp(p->backend_name,"mGBA"));
    const uint8_t iv[]={31,1,17,29,3,2};const uint16_t ev[]={0,1,252,128,125,4};
    for(unsigned permutation=0;permutation<24;permutation++)for(unsigned slot=0;slot<6;slot++) {
        emerald_fixture(p,slot,permutation,false);uint8_t before[100];memcpy(before,at(summary+12,100),100);
        struct training_stats s=training_stats_decode(p,read_memory,NULL);
        assert(s.visible && s.slot==slot && s.species==277 && s.generation==3);
        assert(!memcmp(s.dv,iv,6) && !memcmp(s.ev,ev,sizeof(ev)) && !memcmp(before,at(summary+12,100),100));
        struct battle_state state=battle_decode(p,read_memory,NULL);assert(state.status==BATTLE_OUTSIDE && state.training.visible);
    }
    emerald_fixture(p,0,0,true);struct training_stats s=training_stats_decode(p,read_memory,NULL);assert(s.visible && s.ev[0]==255 && s.ev[1]==255);
    for(unsigned variant=2;variant<=5;variant++) {emerald_fixture(p,0,0,variant);assert(!training_stats_decode(p,read_memory,NULL).visible);}
    const struct {size_t a;uint8_t b;} bad[]={
        {p->main_callback,0},{p->palette_fade+7,0x80},{p->tasks+3*40+4,0},{p->tasks+3*40,0},
        {p->summary_pointer,1},{summary,0},{summary+0x40bc,2},{summary+0x40bd,1},{summary+0x40be,6},
        {summary+0x40bf,4},{summary+0x40c0,0},{summary+0x40c0,2},{p->gba_party_count,0},{p->gba_party_count,7},
        {summary+12+19,3},{summary+12+19,6},{summary+12+84,0},{summary+12+84,101},{summary+12+28,0},{summary+12+32,0},
        {p->gba_party_base+4,0}
    };
    for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++) {
        emerald_fixture(p,0,0,false);*at(bad[i].a,1)=bad[i].b;
        assert(!training_stats_decode(p,read_memory,NULL).visible);
    }
    /* Matching corrupt copies still must fail the checksum/egg/total checks. */
    for(unsigned mode=0;mode<4;mode++) {
        emerald_fixture(p,0,0,false);uint8_t *mon=at(summary+12,100);
        if(mode==0)mon[28]^=1;
        if(mode==1)mon[32+36+7]^=0x40;
        if(mode==2)mon[32+24]^=255;
        if(mode==3)put32(at(p->summary_pointer,4),0x0203f000);
        memcpy(at(p->gba_party_base,100),mon,100);
        assert(!training_stats_decode(p,read_memory,NULL).visible);
    }
    const size_t reads[]={p->main_callback,p->palette_fade+7,p->tasks,p->summary_pointer,summary,summary+0x40bc,p->gba_party_count,summary+12,p->gba_party_base};
    for(unsigned i=0;i<sizeof(reads)/sizeof(*reads);i++){emerald_fixture(p,0,0,false);missing=reads[i];assert(!training_stats_decode(p,read_memory,NULL).visible);missing=0;}
}
int main(void)
{
    kanto_test("ea9bcae617fdf159b045185467ae58b2e4a48b9a");kanto_test("d7037c83e1ae5b39bde3c30787637ba1d4c48ce2");emerald_test();
    puts("Gen 1/3: both Kanto profiles, six slots, parity, 24 encrypted permutations, page/identity/checksum/transition guards passed");
}

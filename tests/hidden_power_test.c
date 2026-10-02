#ifdef NDEBUG
#undef NDEBUG
#endif
#include "hidden_power.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static const enum pokemon_type canonical[]={TYPE_FIGHTING,TYPE_FLYING,TYPE_POISON,TYPE_GROUND,TYPE_ROCK,TYPE_BUG,TYPE_GHOST,TYPE_STEEL,TYPE_FIRE,TYPE_WATER,TYPE_GRASS,TYPE_ELECTRIC,TYPE_PSYCHIC,TYPE_ICE,TYPE_DRAGON,TYPE_DARK};
static void known(unsigned gen,const uint8_t stats[6],enum pokemon_type type,unsigned power)
{
    struct hidden_power hp;uint8_t before[6];memcpy(before,stats,6);
    assert(hidden_power_calculate(gen,stats,&hp) && hp.type==type && hp.power==power);
    assert(!memcmp(stats,before,6));
}
int main(void)
{
    known(2,(const uint8_t[]){0,0,0,0,0,0},TYPE_FIGHTING,31);
    known(2,(const uint8_t[]){15,15,15,15,15,15},TYPE_DARK,70);
    known(2,(const uint8_t[]){6,14,3,0,0,3},TYPE_ELECTRIC,51); /* User's Totodile. */
    known(2,(const uint8_t[]){5,10,5,3,3,12},TYPE_WATER,57);
    known(2,(const uint8_t[]){0,0,0,3,3,0},TYPE_FIGHTING,32); /* Special low bits and floor. */
    known(3,(const uint8_t[]){0,0,0,0,0,0},TYPE_FIGHTING,30);
    known(3,(const uint8_t[]){31,31,31,31,31,31},TYPE_DARK,70);
    known(3,(const uint8_t[]){30,30,30,30,30,30},TYPE_FIGHTING,70);
    known(3,(const uint8_t[]){29,29,29,29,29,29},TYPE_DARK,30);
    known(3,(const uint8_t[]){31,1,17,29,3,2},TYPE_ICE,56);
    bool seen2[19]={0},seen3[19]={0};unsigned min2=255,max2=0,min3=255,max3=0;
    for(unsigned packed=0;packed<65536;packed++) {
        unsigned a=packed>>12,d=(packed>>8)&15,v=(packed>>4)&15,c=packed&15;
        uint8_t stats[]={(uint8_t)(((a&1)<<3)|((d&1)<<2)|((v&1)<<1)|(c&1)),(uint8_t)a,(uint8_t)d,(uint8_t)c,(uint8_t)c,(uint8_t)v};
        struct hidden_power hp;assert(hidden_power_calculate(2,stats,&hp));
        assert(hp.type==canonical[(a%4)*4+d%4]);
        unsigned top=(a>=8 ? 8 : 0)+(d>=8 ? 4 : 0)+(v>=8 ? 2 : 0)+(c>=8 ? 1 : 0);
        assert(hp.power==31+(top*5+c%4)/2);
        assert(hidden_power_gen2_type((uint8_t)a,(uint8_t)d)==hp.type);
        seen2[hp.type]=true;if(hp.power<min2)min2=hp.power;if(hp.power>max2)max2=hp.power;
    }
    const unsigned game_order[]={0,1,2,5,3,4};
    for(unsigned type_mask=0;type_mask<64;type_mask++)for(unsigned power_mask=0;power_mask<64;power_mask++) {
        uint8_t stats[6];
        for(unsigned i=0;i<6;i++)stats[game_order[i]]=(uint8_t)(28+((type_mask>>i)&1)+2*((power_mask>>i)&1));
        struct hidden_power hp;assert(hidden_power_calculate(3,stats,&hp));
        assert(hp.type==canonical[type_mask*15/63] && hp.power==30+power_mask*40/63);
        seen3[hp.type]=true;if(hp.power<min3)min3=hp.power;if(hp.power>max3)max3=hp.power;
        for(unsigned i=0;i<6;i++)stats[i]&=3;
        known(3,stats,hp.type,hp.power); /* High IV bits do not affect either value. */
    }
    assert(min2==31 && max2==70 && min3==30 && max3==70);
    for(unsigned i=0;i<16;i++)assert(seen2[canonical[i]] && seen3[canonical[i]]);
    assert(!seen2[TYPE_NORMAL] && !seen3[TYPE_NORMAL] && !seen2[TYPE_FAIRY] && !seen3[TYPE_FAIRY]);
    struct hidden_power hp;uint8_t stats[6]={0};
    for(unsigned gen=0;gen<=5;gen++)if(gen!=2 && gen!=3)assert(!hidden_power_calculate(gen,stats,&hp) && hp.type==TYPE_NONE && !hp.power);
    assert(!hidden_power_calculate(2,NULL,&hp) && !hidden_power_calculate(3,stats,NULL));
    for(unsigned gen=2;gen<=3;gen++)for(unsigned i=0;i<6;i++) {
        memset(stats,0,6);stats[i]=gen==2 ? 16 : 32;assert(!hidden_power_calculate(gen,stats,&hp) && hp.type==TYPE_NONE && !hp.power);
    }
    assert(hidden_power_gen2_type(16,0)==TYPE_NONE && hidden_power_gen2_type(0,16)==TYPE_NONE);
    puts("Hidden Power: all 65536 Gen 2 spreads, all 4096 Gen 3 relevant bit pairs, known vectors, floors, ranges and invalid inputs passed");
}

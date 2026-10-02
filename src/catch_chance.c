#include "catch_chance.h"
#include <stddef.h>
const char *crystal_ball_name(uint8_t ball)
{
    switch(ball){case 1:return "MASTER";case 2:return "ULTRA";case 4:return "GREAT";
    case 5:return "POKE";case 0x9d:return "HEAVY";case 0x9f:return "LEVEL";
    case 0xa0:return "LURE";case 0xa1:return "FAST";case 0xa4:return "FRIEND";
    case 0xa5:return "MOON";case 0xa6:return "LOVE";default:return NULL;}
}
bool crystal_catch_chance(uint8_t rate,uint16_t hp,uint16_t max_hp,uint8_t status,
                          bool level_ball,uint16_t *out)
{
    if(!out || !hp || !max_hp || hp>max_hp || max_hp>999 || status&0x80)return false;
    /* Valid original status byte: sleep count, or one major ailment. */
    unsigned major=status&0x78;if((major && (major&(major-1))) || (major && (status&7)))return false;
    unsigned final=rate;
    if(!level_ball) {
        unsigned denominator=3u*max_hp,current=2u*hp;
        if(denominator>255){denominator>>=2;current>>=2;if(!(current&255))current=1;}
        denominator&=255;current&=255;
        if(!denominator)return false; /* Original division-by-zero bug, not a modern formula. */
        final=(((denominator-current)&255)*rate/denominator)&255;
        if(!final)final=1;
        /* The second status test in the ROM tests the already-masked value:
         * burn, poison and paralysis never supply their intended +5. */
        if(status&0x27){final+=10;if(final>255)final=255;}
    }
    /* cp b: both less-than and equality succeed; RNG is an 8-bit value. */
    *out=(uint16_t)((final+1u)*10000u/256u);return true;
}

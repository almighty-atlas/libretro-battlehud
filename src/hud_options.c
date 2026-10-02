#include "hud_options.h"
#include <stdlib.h>
#include <string.h>
#define COUNT 6
static const char *keys[COUNT]={"battlehud_types","battlehud_moves","battlehud_training","battlehud_hidden_power","battlehud_party_details","battlehud_layout"};
static const char *names[COUNT]={"BattleHUD: Type icons","BattleHUD: Move effectiveness","BattleHUD: Training values","BattleHUD: Hidden Power","BattleHUD: Nature and ability","BattleHUD: Layout"};
static const char *info[COUNT]={"Show opponent types in battle menus.","Show move hints in FIGHT.","Show DV/IV and training values on the stats page.","Show calculated Hidden Power on the stats page.","Show nature, stat arrows and ability in supported Gen 3 summaries.","Detailed includes training experience/EVs; compact shows DV/IV only. Other displays stay independent."};
static const char *legacy[COUNT]={"BattleHUD: Type icons; enabled|disabled","BattleHUD: Move effectiveness; enabled|disabled","BattleHUD: Training values; enabled|disabled","BattleHUD: Hidden Power; enabled|disabled","BattleHUD: Nature and ability; enabled|disabled","BattleHUD: Layout; detailed|compact"};
struct allocation {void *ptr;struct allocation *next;};
static struct allocation *allocations;
static bool registered;
static void *keep(size_t size)
{
    struct allocation *a=malloc(sizeof(*a));if(!a)return NULL;
    a->ptr=calloc(1,size);if(!a->ptr){free(a);return NULL;}
    a->next=allocations;allocations=a;return a->ptr;
}
/* Bounded scans fail back to the backend's original registration unchanged. */
#define MERGE(NAME,TYPE,FILL) \
static TYPE *NAME(const TYPE *src) { \
    size_t n=0;while(src && n<1024 && src[n].key)n++; \
    if(n==1024)return NULL; \
    TYPE *out=keep((n+COUNT+1)*sizeof(*out));if(!out)return NULL; \
    if(n)memcpy(out,src,n*sizeof(*out)); \
    for(unsigned i=0;i<COUNT;i++){bool collision=false; \
        for(size_t j=0;j<n;j++)if(!strcmp(out[j].key,keys[i]))collision=true; \
        if(collision){continue;} TYPE *d=&out[n++];d->key=keys[i];FILL; \
    }return out; \
}
#define VALUES d->values[0].value=i==5?"detailed":"enabled";d->values[1].value=i==5?"compact":"disabled";d->default_value=d->values[0].value
MERGE(merge_old,struct retro_variable,d->value=legacy[i])
MERGE(merge_v1,struct retro_core_option_definition,d->desc=names[i];d->info=info[i];VALUES)
MERGE(merge_v2_defs,struct retro_core_option_v2_definition,d->desc=names[i];d->info=info[i];VALUES)
static struct retro_core_options_v2 *merge_v2(const struct retro_core_options_v2 *src)
{
    if(!src)return NULL;
    struct retro_core_options_v2 *out=keep(sizeof(*out));if(!out)return NULL;
    out->categories=src->categories;out->definitions=merge_v2_defs(src->definitions);
    return out->definitions?out:NULL;
}
bool hud_options_register(retro_environment_t cb,unsigned cmd,void *data)
{
    void *merged=NULL;
    if(!cb)return false;
    switch(cmd){
    case RETRO_ENVIRONMENT_SET_VARIABLES:merged=merge_old(data);break;
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS:merged=merge_v1(data);break;
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL: {
        const struct retro_core_options_intl *in=data;if(!in)break;
        struct retro_core_options_intl *out=keep(sizeof(*out));if(!out)break;
        out->us=merge_v1(in->us);out->local=in->local?merge_v1(in->local):NULL;
        if(out->us && (!in->local || out->local))merged=out;
        break;
    }
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2:merged=merge_v2(data);break;
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL: {
        const struct retro_core_options_v2_intl *in=data;if(!in)break;
        struct retro_core_options_v2_intl *out=keep(sizeof(*out));if(!out)break;
        out->us=merge_v2(in->us);out->local=in->local?merge_v2(in->local):NULL;
        if(out->us && (!in->local || out->local))merged=out;
        break;
    }
    default:return cb(cmd,data);
    }
    registered=true;
    /* V2's return value describes category support, so preserve it verbatim. */
    return cb(cmd,merged?merged:data);
}
void hud_options_fallback(retro_environment_t cb)
{
    if(cb && !registered)hud_options_register(cb,RETRO_ENVIRONMENT_SET_VARIABLES,NULL);
}
unsigned hud_options_read(retro_environment_t cb)
{
    unsigned flags=HUD_DEFAULT;
    if(!cb)return flags;
    for(unsigned i=0;i<COUNT;i++){
        struct retro_variable v={keys[i],NULL};
        if(!cb(RETRO_ENVIRONMENT_GET_VARIABLE,&v) || !v.value)continue;
        if(i==5){if(!strcmp(v.value,"compact"))flags|=HUD_COMPACT;}
        else if(!strcmp(v.value,"disabled"))flags &= ~(1u<<i);
    }
    /* Do not consume GET_VARIABLE_UPDATE: it belongs to the backend. */
    return flags;
}
void hud_options_clear(void)
{
    while(allocations){struct allocation *a=allocations;allocations=a->next;free(a->ptr);free(a);}
    registered=false;
}

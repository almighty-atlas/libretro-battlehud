#ifdef NDEBUG
#undef NDEBUG
#endif
#include "hud_options.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned calls,updates,kind;
static const char *value;
static void *received;
static bool env(unsigned cmd,void *data)
{
    calls++;
    if(cmd==RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE){updates++;return true;}
    if(cmd==RETRO_ENVIRONMENT_GET_VARIABLE){struct retro_variable *v=data;v->value=value;return value!=NULL;}
    assert(cmd==kind);received=data;return false;
}
static void check_v1(struct retro_core_option_definition *d)
{
    assert(!strcmp(d[0].key,"backend"));assert(!strcmp(d[0].desc,"Original"));
    assert(!strcmp(d[0].values[0].value,"fast"));assert(!strcmp(d[0].default_value,"fast"));
    assert(!strcmp(d[1].key,"battlehud_types"));assert(d[8].key && !d[9].key);
}
int main(void)
{
    struct retro_variable old[]={{"backend","Original; fast|slow"},{NULL,NULL}};
    kind=RETRO_ENVIRONMENT_SET_VARIABLES;
    assert(!hud_options_register(env,kind,old));
    struct retro_variable *v=received;
    assert(v!=old && !strcmp(v[0].value,old[0].value));assert(v[8].key && !v[9].key);assert(!old[1].key);
    unsigned before=calls;hud_options_fallback(env);assert(calls==before);
    struct retro_core_option_definition d[2]={0};d[0].key="backend";d[0].desc="Original";d[0].values[0].value="fast";d[0].default_value="fast";
    kind=RETRO_ENVIRONMENT_SET_CORE_OPTIONS;assert(!hud_options_register(env,kind,d));check_v1(received);
    struct retro_core_options_intl intl={d,d};kind=RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL;
    assert(!hud_options_register(env,kind,&intl));struct retro_core_options_intl *i=received;check_v1(i->us);check_v1(i->local);
    intl.local=NULL;hud_options_register(env,kind,&intl);assert(!((struct retro_core_options_intl *)received)->local);
    struct retro_core_option_v2_category cats[]={{"speed","Speed","Category info"},{0}};
    struct retro_core_option_v2_definition defs[2]={0};defs[0].key="backend";defs[0].desc="Original";defs[0].category_key="speed";defs[0].values[0].value="fast";defs[0].default_value="fast";
    struct retro_core_options_v2 v2={cats,defs};kind=RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2;
    assert(!hud_options_register(env,kind,&v2));struct retro_core_options_v2 *o=received;
    assert(o->categories==cats);assert(!memcmp(&o->definitions[0],&defs[0],sizeof(defs[0])));assert(o->definitions[8].key && !o->definitions[9].key);
    struct retro_core_options_v2_intl vi={&v2,&v2};kind=RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL;
    assert(!hud_options_register(env,kind,&vi));struct retro_core_options_v2_intl *oi=received;assert(oi->us->categories==cats && oi->local->categories==cats);
    /* A backend owns a colliding key; retain its definition, without duplicates. */
    d[0].key="battlehud_types";kind=RETRO_ENVIRONMENT_SET_CORE_OPTIONS;hud_options_register(env,kind,d);
    struct retro_core_option_definition *collision=received;assert(!strcmp(collision[0].desc,"Original"));assert(collision[7].key && !collision[8].key);
    value=NULL;assert(hud_options_read(env)==HUD_DEFAULT);
    value="disabled";assert(hud_options_read(env)==0);
    value="compact";assert(hud_options_read(env)==(HUD_DEFAULT|HUD_COMPACT));
    value="bonus";assert(hud_options_read(env)==(HUD_DEFAULT|HUD_BONUS));
    value="gains";assert(hud_options_read(env)==(HUD_DEFAULT|HUD_GAINS));
    assert(!strcmp(collision[6].values[2].value,"gains"));
    value="details";assert(hud_options_read(env)==(HUD_DEFAULT|HUD_EXTRA_VIEW));
    assert(!strcmp(collision[6].values[3].value,"details"));
    value="invalid";assert(hud_options_read(env)==HUD_DEFAULT);assert(updates==0);
    hud_options_clear();kind=RETRO_ENVIRONMENT_SET_VARIABLES;hud_options_fallback(env);v=received;assert(v[7].key && !v[8].key);hud_options_clear();
    puts("HUD options: legacy/V1/V2, both translations, backend defaults/categories, collision, fallback and updates passed");
}

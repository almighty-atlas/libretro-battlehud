#include "training_progress.h"
#include "party_details.h"
#include <string.h>
bool training_budget(const struct training_stats *m,unsigned *used,unsigned *remaining)
{
    if(!m || !used || !remaining || !m->visible || m->generation!=3)return false;
    unsigned total=0;for(unsigned i=0;i<6;i++){if(m->ev[i]>255)return false;total+=m->ev[i];}
    if(total>510)return false;
    *used=total;*remaining=510-total;return true;
}
static unsigned stat_value(const struct training_stats *m,unsigned stat,unsigned training)
{
    unsigned n=m->generation==3?2*m->base[stat]+m->dv[stat]:2*(m->base[stat]+m->dv[stat]);
    n=(n+training)*m->level/100+(stat==0?m->level+10:5);
    if(m->generation==3 && stat) {
        int nature=gen3_nature_effect(m->nature,stat);
        n=n*(nature==1?110u:nature==-1?90u:100u)/100;
    }
    if(m->generation<3 && n>999)n=999;
    return n;
}
bool training_bonus(const struct training_stats *m,unsigned stat,uint16_t *bonus)
{
    if(!m || !bonus || !m->visible || !m->bonus_known || stat>=6 || !m->level || m->level>100 ||
       m->generation<1 || m->generation>3 || !m->base[stat] || m->dv[stat]>(m->generation==3?31:15))return false;
    unsigned term;
    if(m->generation==3) {
        unsigned used,left;if(!m->nature_known || m->nature>=25 || !training_budget(m,&used,&left))return false;
        if(stat==0 && m->species==303){*bonus=0;return true;} /* Shedinja's HP is always one. */
        term=m->ev[stat]/4;
    } else {
        unsigned root=1;while(root<255 && root*root<m->ev[stat])root++;
        term=root/4;
    }
    *bonus=(uint16_t)(stat_value(m,stat,term)-stat_value(m,stat,0));return true;
}
void training_progress_clear(struct training_progress *p){memset(p,0,sizeof(*p));}
static bool identity_equal(const struct training_stats *a,const struct training_stats *b)
{return a->identity_known && b->identity_known && a->generation==b->generation && !memcmp(a->identity,b->identity,32);}
static bool valid(const struct training_party *s)
{
    if(!s || !s->count || s->count>6)return false;
    for(unsigned i=0;i<s->count;i++) {
        if(!s->mon[i].visible || !s->mon[i].identity_known)return false;
        for(unsigned j=0;j<i;j++)if(identity_equal(&s->mon[i],&s->mon[j]))return false;
    }
    return true;
}
static bool roster_equal(const struct training_party *a,const struct training_party *b)
{
    if(a->count!=b->count)return false;
    for(unsigned i=0;i<a->count;i++)if(!identity_equal(&a->mon[i],&b->mon[i]))return false;
    return true;
}
static bool values_equal(const struct training_party *a,const struct training_party *b)
{
    if(!roster_equal(a,b))return false;
    for(unsigned i=0;i<a->count;i++)if(memcmp(a->mon[i].ev,b->mon[i].ev,sizeof(a->mon[i].ev)))return false;
    return true;
}
bool training_party_read(const struct game_profile *p,training_memory_read read,void *ctx,struct training_party *s)
{
    uint8_t count;if(!p || !read || !s)return false;
    memset(s,0,sizeof(*s));
    if(!read(ctx,p->generation==3?p->gba_party_count:p->party_count,&count,1) || !count || count>6)return false;
    s->count=count;
    for(unsigned i=0;i<count;i++){s->mon[i]=training_party_mon(p,read,ctx,i);if(!s->mon[i].visible)return false;}
    return valid(s);
}
void training_progress_update(struct training_progress *p,const struct training_party *s,int phase)
{
    if(phase<0 || !valid(s)){training_progress_clear(p);return;}
    if(phase==1) {
        if(!p->in_battle) {
            p->before=p->outside;p->eligible=p->outside_known && roster_equal(&p->before,s);
            p->in_battle=true;p->result_known=false;p->pending=false;
        }
        if(p->eligible && !roster_equal(&p->before,s))p->eligible=false;
        /* Any counter decrease invalidates a battle, even if it later rises again. */
        if(p->eligible)for(unsigned i=0;i<s->count;i++)for(unsigned j=0;j<6;j++)
            if(s->mon[i].ev[j]<p->before.mon[i].ev[j])p->eligible=false;
        return;
    }
    if(p->in_battle) {
        p->in_battle=false;p->pending=p->eligible && roster_equal(&p->before,s);p->candidate=*s;
    } else if(p->pending) {
        if(!values_equal(&p->candidate,s)) {p->pending=false;p->result_known=false;}
        else {
            bool monotonic=true;
            for(unsigned i=0;i<s->count;i++)for(unsigned j=0;j<6;j++){
                unsigned before=p->before.mon[i].ev[j],after=s->mon[i].ev[j];
                if(after<before)monotonic=false;
                p->gains[i][j]=(uint16_t)(after>=before?after-before:0);
            }
            p->result=*s;p->result_known=monotonic;p->pending=false;
        }
    } else if(p->result_known && !values_equal(&p->result,s))p->result_known=false;
    p->outside=*s;p->outside_known=true;
}
void training_progress_apply(const struct training_progress *p,struct training_stats *m)
{
    m->gain_known=false;memset(m->gain,0,sizeof(m->gain));
    if(!p->result_known || !m->visible || m->slot>=p->result.count)return;
    unsigned slot=m->slot;
    if(!identity_equal(m,&p->result.mon[slot]) || memcmp(m->ev,p->result.mon[slot].ev,sizeof(m->ev)))return;
    memcpy(m->gain,p->gains[slot],sizeof(m->gain));m->gain_known=true;
}

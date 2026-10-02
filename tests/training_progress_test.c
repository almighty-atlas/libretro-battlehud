#ifdef NDEBUG
#undef NDEBUG
#endif
#include "training_progress.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static struct training_stats mon(unsigned generation,unsigned level)
{
    struct training_stats m={0};m.visible=m.bonus_known=m.nature_known=m.identity_known=true;
    m.generation=(uint8_t)generation;m.level=(uint8_t)level;m.species=277;
    memset(m.base,100,6);m.identity[0]=1;return m;
}
static uint16_t bonus(struct training_stats *m,unsigned stat,unsigned raw)
{uint16_t b=999;m->ev[stat]=(uint16_t)raw;assert(training_bonus(m,stat,&b));return b;}
static void formula_tests(void)
{
    for(unsigned gen=1;gen<=2;gen++) {
        struct training_stats m=mon(gen,100);
        assert(bonus(&m,1,0)==0 && bonus(&m,1,9)==0 && bonus(&m,1,10)==1);
        assert(bonus(&m,1,15)==1 && bonus(&m,1,16)==1 && bonus(&m,1,17)==1);
        assert(bonus(&m,1,49)==1 && bonus(&m,1,50)==2);
        assert(bonus(&m,1,65025)==63 && bonus(&m,1,65535)==63);
        /* Every Stat EXP value, independently bracketed by perfect squares. */
        for(unsigned root=0;root<=255;root++) {
            unsigned first=root?((root-1)*(root-1)+1):0,last=root==255?65535:root*root;
            for(unsigned exp=first;exp<=last;exp++)assert(bonus(&m,0,exp)==root/4);
        }
        m.level=50;assert(bonus(&m,1,65535)==31);
        m.level=1;assert(bonus(&m,1,65535)==0);
        /* An existing fractional stat can make a small training term cross a boundary. */
        m.base[1]=49;m.level=50;assert(bonus(&m,1,50)==1);
        m.level=5;m.base[1]=39;assert(bonus(&m,1,10)==0);assert(bonus(&m,1,50)==1);
        m.level=0;uint16_t b;assert(!training_bonus(&m,1,&b));
    }
    struct training_stats m=mon(3,100);unsigned used,left;
    assert(training_budget(&m,&used,&left) && used==0 && left==510);
    assert(bonus(&m,1,1)==0 && bonus(&m,1,3)==0 && bonus(&m,1,4)==1);
    assert(bonus(&m,1,252)==63 && bonus(&m,1,255)==63);
    m.ev[0]=255;assert(training_budget(&m,&used,&left) && used==510 && left==0);
    m.ev[2]=1;assert(!training_budget(&m,&used,&left));m.ev[2]=0;m.ev[0]=0;
    m.level=50;assert(bonus(&m,1,252)==31);m.level=1;assert(bonus(&m,1,252)==0);
    m.base[1]=39;m.level=5;assert(bonus(&m,1,4)==0 && bonus(&m,1,8)==1);
    m.base[1]=100;m.level=100;m.nature=3;assert(bonus(&m,1,252)==69);assert(bonus(&m,3,252)==57);
    memset(m.ev,0,sizeof(m.ev));m.nature=0;m.species=303;assert(bonus(&m,0,252)==0);
    m.bonus_known=false;uint16_t b;assert(!training_bonus(&m,0,&b));
    m.bonus_known=true;m.nature_known=false;assert(!training_bonus(&m,1,&b));
    m.nature_known=true;m.generation=0;assert(!training_bonus(&m,1,&b));
}
static struct training_party party(unsigned gen)
{
    struct training_party p={0};p.count=2;p.mon[0]=mon(gen,12);p.mon[1]=mon(gen,24);
    p.mon[1].identity[0]=2;p.mon[1].slot=1;return p;
}
static void complete(struct training_progress *t,struct training_party *p)
{
    training_progress_update(t,p,1);p->mon[0].ev[1]+=4;p->mon[1].ev[5]+=7;
    training_progress_update(t,p,1);training_progress_update(t,p,0);assert(!t->result_known);
    training_progress_update(t,p,0);assert(t->result_known);
    struct training_stats s=p->mon[0];training_progress_apply(t,&s);assert(s.gain_known && s.gain[1]==4 && s.gain[5]==0);
    s=p->mon[1];training_progress_apply(t,&s);assert(s.gain_known && s.gain[5]==7);
}
static void tracking_tests(void)
{
    for(unsigned gen=1;gen<=3;gen++) {
        struct training_progress t={0};struct training_party p=party(gen);
        training_progress_update(&t,&p,0);complete(&t,&p);
        /* Stats/level changes are not training; the last observed raw gains stay intact. */
        p.mon[0].level++;training_progress_update(&t,&p,0);assert(t.result_known);
        complete(&t,&p);assert(t.gains[0][1]==4); /* Latest battle only, not cumulative. */
        for(unsigned scenario=0;scenario<9;scenario++) {
            training_progress_clear(&t);p=party(gen);training_progress_update(&t,&p,0);training_progress_update(&t,&p,1);
            if(scenario==0){struct training_stats tmp=p.mon[0];p.mon[0]=p.mon[1];p.mon[1]=tmp;} /* reorder */
            if(scenario==1)p.count=3; /* catch: invalid third member */
            if(scenario==2)p.count=1; /* deposit */
            if(scenario==3)p.mon[0].identity[0]=3; /* same slot, different Pokémon */
            if(scenario==4)p.mon[1].identity[0]=1; /* ambiguous clones */
            if(scenario==5)p.mon[0].identity_known=false;
            if(scenario==6)training_progress_clear(&t); /* reset/load/ROM switch */
            if(scenario==7)training_progress_update(&t,NULL,1); /* unavailable read */
            if(scenario==8)training_progress_update(&t,&p,-1); /* unsupported battle */
            training_progress_update(&t,&p,1);p.mon[0].ev[1]+=10;
            training_progress_update(&t,&p,0);training_progress_update(&t,&p,0);assert(!t.result_known);
        }
        training_progress_clear(&t);p=party(gen);p.mon[0].ev[1]=10;
        training_progress_update(&t,&p,0);training_progress_update(&t,&p,1);
        p.mon[0].ev[1]=9;training_progress_update(&t,&p,1);p.mon[0].ev[1]=12;
        training_progress_update(&t,&p,0);training_progress_update(&t,&p,0);assert(!t.result_known);
        training_progress_clear(&t);p=party(gen);training_progress_update(&t,&p,0);complete(&t,&p);
        p.mon[0].ev[1]++;training_progress_update(&t,&p,0);assert(!t.result_known); /* vitamin/new state */
        training_progress_clear(&t);p=party(gen);training_progress_update(&t,&p,1);
        p.mon[0].ev[1]++;training_progress_update(&t,&p,0);training_progress_update(&t,&p,0);assert(!t.result_known); /* starts mid-battle */
        training_progress_clear(&t);p=party(gen);training_progress_update(&t,&p,0);training_progress_update(&t,&p,1);
        training_progress_update(&t,&p,0);p.mon[0].ev[1]++;training_progress_update(&t,&p,0);assert(!t.result_known); /* unstable finish */
    }
}
int main(void)
{formula_tests();tracking_tests();puts("Training progress: Gen 1/2 exhaustive Stat EXP, Gen 3 limits/rounding/nature/Shedinja, all generations' observed gains and invalidation guards passed");}

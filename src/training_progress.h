#ifndef BATTLEHUD_TRAINING_PROGRESS_H
#define BATTLEHUD_TRAINING_PROGRESS_H
#include "training_stats.h"
/* Calculated stat difference: trained minus zero-training, same level/DVs/IVs/nature. */
bool training_bonus(const struct training_stats *mon,unsigned stat,uint16_t *bonus);
bool training_budget(const struct training_stats *mon,unsigned *used,unsigned *remaining);
struct training_party { unsigned count;struct training_stats mon[6]; };
struct training_progress {
    bool outside_known, in_battle, eligible, pending, result_known;
    struct training_party outside, before, candidate, result;
    uint16_t gains[6][6];
};
void training_progress_clear(struct training_progress *progress);
/* Once per emulated frame: phase 0 outside, 1 validated ordinary battle, -1 unknown. */
void training_progress_update(struct training_progress *progress,const struct training_party *party,int phase);
bool training_party_read(const struct game_profile *,training_memory_read,void *,struct training_party *);
void training_progress_apply(const struct training_progress *,struct training_stats *);
#endif

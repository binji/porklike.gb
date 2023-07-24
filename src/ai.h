#ifndef AI_H_
#define AI_H_

#include "common.h"

u8 ai_run_tasks(void);
u8 ai_run_mob_task(u8 index);
u8 ai_dobump(u8 index);
u8 ai_tcheck(u8 index);
u8 ai_getnextstep(u8 index);
u8 ai_getnextstep_rev(u8 index);
void calcdist_ai(u8 from, u8 to) __sdcccall(0);

#endif // AI_H_

#ifndef TARGETING_H_
#define TARGETING_H_

#include "common.h"

void targeting_init(void);
void targeting_update(void);

extern u8 is_targeting;
extern Dir target_dir;

#endif // TARGETING_H_

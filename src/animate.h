#ifndef ANIMATE_H_
#define ANIMATE_H_

#include "common.h"

void animate_init(void);
void animate_begin(void);
void animate_end(void);
u8 animate(void);

extern u8 dirty_code[];

#endif // ANIMATE_H_

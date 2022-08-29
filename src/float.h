#ifndef FLOAT_H_
#define FLOAT_H_

#include "common.h"

#define FLOAT_FOUND 0xe
#define FLOAT_LOST 0xf

void float_hide(void);
void float_add(u8 pos, u8 tile);
void float_update(void);

extern u8* next_float;

#endif // FLOAT_H_

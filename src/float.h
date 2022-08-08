#ifndef FLOAT_H_
#define FLOAT_H_

#include "common.h"
#include "pickup.h"

#define FLOAT_FOUND 0xe
#define FLOAT_LOST 0xf

void float_hide(void);
void float_add(u8 pos, u8 tile, u8 prop);
void float_blind(void);
void float_pickup(PickupType);
void float_update(void);

#endif // FLOAT_H_

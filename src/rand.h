#ifndef RAND_H_
#define RAND_H_

#include "common.h"

#define XRND_10_PERCENT() (xrnd() < 26)
#define XRND_12_5_PERCENT() ((xrnd() & 7) == 0)
#define XRND_20_PERCENT() (xrnd() < 51)
#define XRND_25_PERCENT() ((xrnd() & 3) == 0)
#define XRND_33_PERCENT() (xrnd() < 85)
#define XRND_50_PERCENT() (xrnd() & 1)

u8 xrnd(void) PRESERVES_REGS(b, c);
void xrnd_init(u16) PRESERVES_REGS(b, c);
void xrnd_mix(u8) PRESERVES_REGS(b, c);
u8 randint(u8 mx);

extern u16 xrnd_seed;

#endif // RAND_H_

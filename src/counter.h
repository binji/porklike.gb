#ifndef COUNTER_H_
#define COUNTER_H_

#include "common.h"

typedef struct Counter {
  u8 start;   // First digit index in data[]
  u8 data[5]; // Digits as tile indexes, left padded
} Counter;

void counter_zero(Counter* c) PRESERVES_REGS(b, c);
void counter_thirty(Counter* c) PRESERVES_REGS(b, c);
void counter_inc(Counter* c);
void counter_dec(Counter* c);
void counter_out(Counter* c, u16 vram);

#endif // COUNTER_H_

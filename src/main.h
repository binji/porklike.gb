#ifndef MAIN_H_
#define MAIN_H_

#ifdef __SDCC
#include <gb/gb.h>
#include <gb/cgb.h>
#endif

#include "common.h"

void clear_wram(void) PRESERVES_REGS(b, c);

extern u8 joy, lastjoy, newjoy;

#endif // MAIN_H_

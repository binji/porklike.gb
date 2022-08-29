#ifndef MAIN_H_
#define MAIN_H_

#ifdef __SDCC
#include <gb/gb.h>
#endif

#include "common.h"

extern const u8 drop_diff[];

extern const u8 dead_map[];
extern const u8 win_map[];
extern const u8 stats_map[];

void titlescreen(void);
void clear_wram(void) PRESERVES_REGS(b, c);

void hide_sprites(void);
void fadeout(void);
void fadein(void);

void mapgen(void);

extern u8 joy, lastjoy, newjoy;

extern Map sigmap;
extern Map tempmap;

extern u8 dirty[];

#endif // MAIN_H_

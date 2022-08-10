#ifndef MAIN_H_
#define MAIN_H_

#ifdef __SDCC
#include <gb/gb.h>
#endif

#include "common.h"

#define MAX_ROOMS 4

extern const u8 fadepal[];
extern const u8 obj_pal1[];

extern const u8 float_diff_y[];

extern const u8 drop_diff[];

extern const u8 msg_tiles[];

extern const u8 dead_map[];
extern const u8 win_map[];
extern const u8 stats_map[];

void titlescreen(void);
void clear_wram(void) PRESERVES_REGS(b, c);
u16 drag(u16 x);

void hide_sprites(void);
void fadeout(void);
void fadein(void);

void mapgen(void);

extern u8 joy, lastjoy, newjoy;

extern Map sigmap;
extern Map tempmap;

extern u8 room_pos[];
extern u8 room_w[];
extern u8 room_h[];
extern u8 room_avoid[];

extern u8 dirty[];

extern u8 start_room;
extern u16 steps;

#endif // MAIN_H_

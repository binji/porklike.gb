#ifndef ANIMATE_H_
#define ANIMATE_H_

#include "common.h"

void animate_init(void);
void animate_begin(void);
void animate_end(void);
u8 animate(void);

extern u8 anim_tiles[];
extern u8 *anim_tile_ptr;

extern u8 dirty_code[];
extern u8 *dirty_ptr;

extern u8 dirty_saw[];
extern u8 *dirty_saw_ptr;

#endif // ANIMATE_H_

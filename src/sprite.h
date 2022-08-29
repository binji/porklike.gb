#ifndef SPRITE_H_
#define SPRITE_H_

#include "common.h"

void sprite_hide(void);
void sprite_animate_begin(void);
void sprite_animate_end(void);
void sprite_update(void);

extern u8 *next_sprite, *last_next_sprite;

#endif // SPRITE_H_

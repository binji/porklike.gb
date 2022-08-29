#include <gb/gb.h>

#include "float.h"

#include "inventory.h"
#include "sprite.h"

#pragma bank 1

#define MAX_FLOATS 8   /* For now; we'll probably want more */

extern const u8 float_diff_y[];

OAM_item_t float_sprites[MAX_FLOATS];
u8* next_float;

void float_hide(void) {
  next_float = (u8*)float_sprites;
}

void float_add(u8 pos, u8 tile) {
  if (next_float != (u8*)(float_sprites + MAX_FLOATS)) {
    *next_float++ = POS_TO_Y(pos);
    *next_float++ = POS_TO_X(pos);
    *next_float++ = tile;
    *next_float++ = FLOAT_FRAMES; // hijack prop for float time
  }
}

void float_update(void) {
  // Draw float sprites and remove them if timed out
  u8 *spr = (u8 *)float_sprites;
  while (spr != next_float) {
    if (--spr[3] == 0) { // float time
      next_float -= 4;
      if (spr != next_float) {
        spr[0] = next_float[0];
        spr[1] = next_float[1];
        spr[2] = next_float[2];
        spr[3] = next_float[3];
      }
      continue;
    } else if (spr[0] > 16) {
      // Update Y coordinate based on float time
      spr[0] -= float_diff_y[spr[3]];
    }

    // Copy float sprite
    if (*spr <= INV_TOP_Y()) {
      *next_sprite++ = *spr++;
      *next_sprite++ = *spr++;
      *next_sprite++ = *spr++;
      *next_sprite++ = 0; // Stuff 0 in for prop
      spr++;              // And increment past the float timer
    } else {
      spr += 4;
    }
  }
}


#include <gb/gb.h>

#include "float.h"

#include "inventory.h"
#include "mob.h"
#include "pickup.h"
#include "sprite.h"

#pragma bank 1

#define MAX_FLOATS 8   /* For now; we'll probably want more */

#define FLOAT_BLIND_X_OFFSET 0xfb
#define TILE_BLIND 0x37

extern const u8 float_pick_type_tiles[];
extern const u8 float_pick_type_start[];
extern const u8 float_pick_type_x_offset[];

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

void float_blind(void) {
  u8 i, x, y;
  x = POS_TO_X(mob_pos[PLAYER_MOB]) + FLOAT_BLIND_X_OFFSET;
  y = POS_TO_Y(mob_pos[PLAYER_MOB]);
  for (i = 0; i < 3; ++i) {
    *next_float++ = y;
    *next_float++ = x;
    *next_float++ = TILE_BLIND + i;
    *next_float++ = FLOAT_FRAMES; // hijack prop for float timer
    x += 8;
  }
}

void float_pickup(PickupType ptype) {
  u8 i;
  u8 pos = mob_pos[PLAYER_MOB];
  u8 flt_start = float_pick_type_start[ptype];
  u8 flt_end = float_pick_type_start[ptype + 1];
  u8 x = POS_TO_X(pos) + float_pick_type_x_offset[ptype];
  u8 y = POS_TO_Y(pos);

  for (i = flt_start; i < flt_end; ++i) {
    *next_float++ = y;
    *next_float++ = x;
    *next_float++ = float_pick_type_tiles[i];
    *next_float++ = FLOAT_FRAMES; // hijack prop for float timer
    x += 8;
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


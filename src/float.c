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

u8 float_times[MAX_FLOATS];
u8* next_float_time;

void float_hide(void) {
  next_float = (u8*)float_sprites;
  next_float_time = float_times;
}

void float_add(u8 pos, u8 tile, u8 prop) {
  if (next_float != (u8*)(float_sprites + MAX_FLOATS)) {
    *next_float++ = POS_TO_Y(pos);
    *next_float++ = POS_TO_X(pos);
    *next_float++ = tile;
    *next_float++ = prop;
    *next_float_time++ = FLOAT_FRAMES;
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
    *next_float++ = 3;
    *next_float_time++ = FLOAT_FRAMES;
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
    *next_float++ = 3;
    *next_float_time++ = FLOAT_FRAMES;
    x += 8;
  }
}

void float_update(void) {
  // Draw float sprites and remove them if timed out
  u8 *flt = (u8 *)float_sprites;
  u8 *flt_time = float_times;
  while (flt != next_float) {
    if (--*flt_time == 0) { // float time
      next_float -= 4;
      --next_float_time;
      if (flt != next_float) {
        flt[0] = next_float[0];
        flt[1] = next_float[1];
        flt[2] = next_float[2];
        flt[3] = next_float[3];
        *flt_time = *next_float_time;
      }
      continue;
    } else if (flt[0] > 16) {
      // Update Y coordinate based on float time
      flt[0] -= float_diff_y[*flt_time];
    }

    // Copy float sprite
    if (*flt <= INV_TOP_Y()) {
      *next_sprite++ = *flt++;
      *next_sprite++ = *flt++;
      *next_sprite++ = *flt++;
      *next_sprite++ = *flt++;
    } else {
      flt += 4;
    }
    flt_time++;
  }
}


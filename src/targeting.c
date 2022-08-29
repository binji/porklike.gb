#include <gb/gb.h>

#include "targeting.h"

#include "inventory.h"
#include "mob.h"
#include "pickup.h"
#include "sprite.h"

#define TARGET_FRAMES 2
#define TARGET_OFFSET 3

#define TILE_ARROW_L 0x9

#pragma bank 1

u8 is_targeting;
Dir target_dir;
u8 inv_target_timer;
u8 inv_target_frame;

void targeting_init(void) {
  inv_target_timer = TARGET_FRAMES;
}

void targeting_update(void) {
  // Draw the targeting arrow
  if (is_targeting) {
    if (--inv_target_timer == 0) {
      inv_target_timer = TARGET_FRAMES;
      ++inv_target_frame;
    }

    u8 dir;
    u8 ppos = mob_pos[PLAYER_MOB];
    u8 valid = validmap[ppos];

    for (dir = 0; dir < 4; ++dir) {
      if ((valid & dirvalid[dir]) &&
          (dir == target_dir || inv_selected_pick == PICKUP_TYPE_SPIN)) {
        u8 pos = POS_DIR(ppos, dir);
        u8 x = POS_TO_X(pos);
        u8 y = POS_TO_Y(pos);
        u8 delta = TARGET_OFFSET + pickbounce[inv_target_frame & 7];

        if (dir == DIR_LEFT) {
          x -= delta;
        } else if (dir == DIR_RIGHT) {
          x += delta;
        } else if (dir == DIR_UP) {
          y -= delta;
        } else if (dir == DIR_DOWN) {
          y += delta;
        }

        if (y <= INV_TOP_Y()) {
          *next_sprite++ = y;
          *next_sprite++ = x;
          *next_sprite++ = TILE_ARROW_L + dir;
          *next_sprite++ = 0;
        }
      }
    }
  }
}

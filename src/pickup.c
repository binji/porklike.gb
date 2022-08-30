#include <gb/gb.h>

#include "mob.h"

#include "gameplay.h"
#include "inventory.h"
#include "pickup.h"
#include "rand.h"
#include "sprite.h"

#pragma bank 1

#define MAX_PICKS 16   /* XXX Figure out what this should be */

#define PICK_HOP_FRAMES 8
#define PICKUP_FRAMES 6

#define PICK_FIELDS 10
PickupType pick_type[MAX_PICKS];
u8 pick_tile[MAX_PICKS]; // Actual tile index
u8 pick_pos[MAX_PICKS];
u8 pick_anim_frame[MAX_PICKS]; // Index into pick_type_anim_frames
u8 pick_anim_timer[MAX_PICKS]; // 0..pick_type_anim_speed[type]
u8 pick_x[MAX_PICKS];          // x location of sprite
u8 pick_y[MAX_PICKS];          // y location of sprite
u8 pick_dx[MAX_PICKS];         // dx moving sprite
u8 pick_dy[MAX_PICKS];         // dy moving sprite
u8 pick_move_timer[MAX_PICKS]; // timer for sprite animation
u8 num_picks;

extern const u8 pick_type_anim_frames[];
extern const u8 pick_type_anim_start[];
extern const u8 pick_type_sprite_tile[];

void pickhop(u8 index, u8 newpos) {
  u8 pos, oldx, oldy, newx, newy;
  pos = pick_pos[index];
  pick_x[index] = oldx = POS_TO_X(pos);
  pick_y[index] = oldy = POS_TO_Y(pos);
  newx = POS_TO_X(newpos);
  newy = POS_TO_Y(newpos);

  pick_dx[index] = (s8)(newx - oldx) >> 3;
  pick_dy[index] = (s8)(newy - oldy) >> 3;

  pick_move_timer[index] = PICK_HOP_FRAMES;
  pick_pos[index] = newpos;
  pickmap[pos] = 0;
  pickmap[newpos] = index + 1;
}

void addpick(PickupType type, u8 pos) NONBANKED {
  if (num_picks != MAX_PICKS) {
    pick_type[num_picks] = type;
    pick_anim_frame[num_picks] = pick_type_anim_start[type];
    pick_anim_timer[num_picks] = 1;
    pick_pos[num_picks] = pos;
    pick_move_timer[num_picks] = 0;
    ++num_picks;
    pickmap[pos] = num_picks; // index+1
  }
}

void delpick(u8 index) NONBANKED {  // XXX: only used in bank 1
  pickmap[pick_pos[index]] = 0;
  --num_picks;
  if (index != num_picks) {
    // Copy all fields (in SoA) from num_picks => index.
    u8* dst = (u8*)pick_type + index;
    u8* src = (u8*)pick_type + num_picks;
    for (u8 i = 0; i < PICK_FIELDS; ++i) {
      *dst = *src;
      src += MAX_PICKS;
      dst += MAX_PICKS;
    }
    pickmap[pick_pos[index]] = index + 1;
  }
}

u8 animate_pickups(void) {
  u8 i, dotile, sprite, frame, animdone;

  animdone = 1;

  // Loop through all pickups
  for (i = 0; i < num_picks; ++i) {
    // Don't draw the pickup if it is fogged or a mob is standing on top of it
    if (fogmap[pick_pos[i]] || mobmap[pick_pos[i]]) { continue; }

    dotile = 0;

    sprite = pick_type_sprite_tile[pick_type[i]];
    if (--pick_anim_timer[i] == 0) {
      pick_anim_timer[i] = PICKUP_FRAMES;

      // bounce the sprite (if not a heart or key)
      if (sprite) {
        pick_y[i] += pickbounce[pick_anim_frame[i] & 7];
      }

      // All pickups have 8 frames
      if ((++pick_anim_frame[i] & 7) == 0) {
        pick_anim_frame[i] -= 8;
      }

      dotile = 1;
    }

    // Don't display pickups if the window would cover it; the bottom of the
    // sprite would be pick_y[i] + 8, but sprites are offset by 16 already (as
    // required by the hardware), so the actual position is pick_y[i] - 8.
    // Since the pickups bounce by 1 pixel up and down, we also display them at
    // WY_REG + 7 so they don't disappear while bouncing.
    if (sprite && pick_y[i] <= INV_TOP_Y()) {
      *next_sprite++ = pick_y[i];
      *next_sprite++ = pick_x[i];
      *next_sprite++ = sprite;
      *next_sprite++ = 0;
    }

    if (dotile || pick_move_timer[i]) {
      pick_tile[i] = frame = pick_type_anim_frames[pick_anim_frame[i]];
      if (pick_move_timer[i]) {
        animdone = 0;
        if (pick_y[i] <= INV_TOP_Y()) {
          *next_sprite++ = pick_y[i];
          *next_sprite++ = pick_x[i];
          *next_sprite++ = frame;
          *next_sprite++ = 0;
        }

        pick_x[i] += pick_dx[i];
        pick_y[i] += pick_dy[i] + hopy12[--pick_move_timer[i]];

        if (pick_move_timer[i] == 0) {
          dirty_tile(pick_pos[i]);
        }
      } else {
        dirty_tile(pick_pos[i]);
      }
    }
  }

  return animdone;
}

void droppick(PickupType type, u8 pos) {
  u8 droppos = dropspot(pos);
  addpick(type, pos);
  pickhop(num_picks - 1, droppos);
}

void droppick_rnd(u8 pos) {
#if 1
  droppick(randint(10) + 2, pos);
#else
  droppick(PICKUP_TYPE_SPIN, pos);
#endif
}

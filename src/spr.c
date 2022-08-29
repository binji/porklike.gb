#include <gb/gb.h>
#include <string.h>

#include "gameplay.h"
#include "inventory.h"
#include "spr.h"
#include "sprite.h"

#pragma bank 1

#define MAX_SPRS 32 /* Animation sprites for explosions, etc. */

u16 drag(u16 x);

SprType spr_type[MAX_SPRS];
u8 spr_anim_frame[MAX_SPRS];            // Actual sprite tile (not index)
u8 spr_anim_timer[MAX_SPRS];            // 0..spr_anim_speed
u8 spr_anim_speed[MAX_SPRS];            //
u16 spr_x[MAX_SPRS], spr_y[MAX_SPRS];   // 8.8 fixed point
u16 spr_dx[MAX_SPRS], spr_dy[MAX_SPRS]; // 8.8 fixed point
u8 spr_timer[MAX_SPRS];
u8 spr_drag[MAX_SPRS];
u8 spr_trigger_val[MAX_SPRS]; // Which value to use for trigger action
u8 spr_prop[MAX_SPRS];                  // Hardware sprite property
u8 num_sprs;

void spr_hide(void) NONBANKED {
  num_sprs = 0;
}

void spr_update(void) {
  u8 i;

  // Draw sprs
  for (i = 0; i < num_sprs;) {
    if (--spr_timer[i] == 0) {
      trigger_spr(spr_type[i], spr_trigger_val[i]);

      if (--num_sprs != i) {
        spr_type[i] = spr_type[num_sprs];
        spr_anim_frame[i] = spr_anim_frame[num_sprs];
        spr_anim_timer[i] = spr_anim_timer[num_sprs];
        spr_anim_speed[i] = spr_anim_speed[num_sprs];
        spr_x[i] = spr_x[num_sprs];
        spr_y[i] = spr_y[num_sprs];
        spr_dx[i] = spr_dx[num_sprs];
        spr_dy[i] = spr_dy[num_sprs];
        spr_drag[i] = spr_drag[num_sprs];
        spr_timer[i] = spr_timer[num_sprs];
        spr_trigger_val[i] = spr_trigger_val[num_sprs];
        spr_prop[i] = spr_prop[num_sprs];
      }
    } else {
      if (--spr_anim_timer[i] == 0) {
        ++spr_anim_frame[i];
        spr_anim_timer[i] = spr_anim_speed[i];
      }
      spr_x[i] += spr_dx[i];
      spr_y[i] += spr_dy[i];
      if (spr_drag[i]) {
        spr_dx[i] = drag(spr_dx[i]);
        spr_dy[i] = drag(spr_dy[i]);
      }

      u8 y = spr_y[i] >> 8;
      if (y <= INV_TOP_Y()) {
        *next_sprite++ = y;
        *next_sprite++ = spr_x[i] >> 8;
        *next_sprite++ = spr_anim_frame[i];
        *next_sprite++ = spr_prop[i];
      }
      ++i;
    }
  }
}

u8 spr_add(u8 speed, u16 x, u16 y, u16 dx, u16 dy, u8 drag, u8 timer, u8 prop) {
  if (num_sprs == MAX_SPRS) {
    // Just overwrite the last spr
    --num_sprs;
  }
  spr_anim_timer[num_sprs] = spr_anim_speed[num_sprs] = speed;
  spr_x[num_sprs] = x;
  spr_y[num_sprs] = y;
  spr_dx[num_sprs] = dx;
  spr_dy[num_sprs] = dy;
  spr_drag[num_sprs] = drag;
  spr_timer[num_sprs] = timer;
  spr_prop[num_sprs] = prop;
  return num_sprs++;
}


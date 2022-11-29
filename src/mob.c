#include <gb/gb.h>

#include "mob.h"

#include "ai.h"
#include "gameplay.h"
#include "inventory.h"
#include "palette.h"
#include "sprite.h"

#pragma bank 1

#define MAX_DEAD_MOBS 4
#define MAX_MOBS 32    /* XXX Figure out what this should be */

#define WALK_FRAMES 8
#define BUMP_FRAMES 4
#define MOB_HOP_FRAMES 8
#define DEAD_MOB_FRAMES 10

#define TILE_FLIP_DIFF 0x21
#define TILE_MOB_FLASH_DIFF 0x46

#define MOB_FIELDS 23
MobType mob_type[MAX_MOBS];
u8 mob_tile[MAX_MOBS];       // Actual tile index
u8 mob_anim_frame[MAX_MOBS]; // Index into mob_type_anim_frames
u8 mob_anim_timer[MAX_MOBS]; // 0..mob_anim_speed[type], how long between frames
u8 mob_anim_speed[MAX_MOBS]; // current anim speed (changes when moving)
u8 mob_pos[MAX_MOBS];
u8 mob_x[MAX_MOBS];                    // x location of sprite
u8 mob_y[MAX_MOBS];                    // y location of sprite
u8 mob_dx[MAX_MOBS];                   // dx moving sprite
u8 mob_dy[MAX_MOBS];                   // dy moving sprite
u8 mob_move_timer[MAX_MOBS];           // timer for sprite animation
MobAnimState mob_anim_state[MAX_MOBS]; // sprite animation state
u8 mob_flip[MAX_MOBS];                 // 0=facing right 1=facing left
MobAI mob_task[MAX_MOBS];              // AI routine
u8 mob_target_pos[MAX_MOBS];           // where the mob last saw the player
u8 mob_ai_cool[MAX_MOBS]; // cooldown time while mob is searching for player
u8 mob_active[MAX_MOBS];  // 0=inactive 1=active
u8 mob_charge[MAX_MOBS];  // only used by queen
u8 mob_hp[MAX_MOBS];
u8 mob_flash[MAX_MOBS];
u8 mob_stun[MAX_MOBS];
u8 mob_trigger[MAX_MOBS]; // Trigger a step action after this anim finishes?
u8 mob_vis[MAX_MOBS];     // Whether mob is visible on fogged tiles (for ghosts)
u8 num_mobs;
u8 key_mob;

u8 dmob_x[MAX_DEAD_MOBS];
u8 dmob_y[MAX_DEAD_MOBS];
u8 dmob_tile[MAX_DEAD_MOBS];
u8 dmob_prop[MAX_DEAD_MOBS];
u8 dmob_timer[MAX_DEAD_MOBS];
u8 num_dead_mobs;

void mobdir(u8 index, u8 dir) {
  if (dir == DIR_LEFT) {
    mob_flip[index] = 1;
  } else if (dir == DIR_RIGHT) {
    mob_flip[index] = 0;
  }
}

void mobwalk(u8 index, u8 dir) {
  u8 pos, newpos;

  pos = mob_pos[index];
  mob_x[index] = POS_TO_X(pos);
  mob_y[index] = POS_TO_Y(pos);
  mob_dx[index] = dirx[dir];
  mob_dy[index] = diry[dir];
  dirty_tile(pos);

  mob_move_timer[index] = WALK_FRAMES;
  mob_anim_state[index] = MOB_ANIM_STATE_WALK;
  mob_pos[index] = newpos = POS_DIR(pos, dir);
  mob_anim_speed[index] = mob_anim_timer[index] = 3;
  mob_trigger[index] = 1;
  mobmap[pos] = 0;
  mobmap[newpos] = index + 1;
  mobdir(index, dir);
}

void mobbump(u8 index, u8 dir) {
  u8 pos = mob_pos[index];
  mob_x[index] = POS_TO_X(pos);
  mob_y[index] = POS_TO_Y(pos);
  mob_dx[index] = dirx[dir];
  mob_dy[index] = diry[dir];
  dirty_tile(pos);

  mob_move_timer[index] = BUMP_FRAMES;
  mob_anim_state[index] = MOB_ANIM_STATE_BUMP1;
  mobdir(index, dir);
}

void mobhop(u8 index, u8 newpos) {
  u8 pos = mob_pos[index];
  mobhopnew(index, newpos);
  mob_tile[index] = 0;
  dirty_tile(pos);
}

void mobhopnew(u8 index, u8 newpos) {
  u8 pos, oldx, oldy, newx, newy;
  pos = mob_pos[index];
  mob_x[index] = oldx = POS_TO_X(pos);
  mob_y[index] = oldy = POS_TO_Y(pos);
  newx = POS_TO_X(newpos);
  newy = POS_TO_Y(newpos);

  mob_dx[index] = (s8)(newx - oldx) >> 3;
  mob_dy[index] = (s8)(newy - oldy) >> 3;

  mob_move_timer[index] = MOB_HOP_FRAMES;
  mob_anim_state[index] = MOB_ANIM_STATE_HOP4;
  mob_pos[index] = newpos;
  mobmap[pos] = 0;
  mobmap[newpos] = index + 1;
}

void addmob(MobType type, u8 pos) NONBANKED {
  if (num_mobs != MAX_MOBS) {
    mob_type[num_mobs] = type;
    mob_anim_frame[num_mobs] = mob_type_anim_start[type];
    mob_anim_timer[num_mobs] = 1;
    mob_anim_speed[num_mobs] = mob_type_anim_speed[type];
    mob_pos[num_mobs] = pos;
    mob_x[num_mobs] = POS_TO_X(pos);
    mob_y[num_mobs] = POS_TO_Y(pos);
    mob_move_timer[num_mobs] = 0;
    mob_anim_state[num_mobs] = MOB_ANIM_STATE_NONE;
    mob_flip[num_mobs] = 0;
    mob_task[num_mobs] = mob_type_ai_wait[type];
    mob_target_pos[num_mobs] = 0;
    mob_ai_cool[num_mobs] = 0;
    mob_active[num_mobs] = 0;
    mob_charge[num_mobs] = 0;
    mob_hp[num_mobs] = mob_type_hp[type];
    mob_flash[num_mobs] = 0;
    mob_stun[num_mobs] = 0;
    mob_trigger[num_mobs] = 0;
    mob_vis[num_mobs] = 1;
    ++num_mobs;
    mobmap[pos] = num_mobs; // index+1
  }
}

void delmob(u8 index) NONBANKED {  // XXX: only used in bank 1
  mobmap[mob_pos[index]] = 0;

  // If this mob was the key mob, then there is no more key mob.
  if (index + 1 == key_mob) {
    key_mob = 0;
  } else if (num_mobs == key_mob) {
    // If the last mob was the key mob, update it to index
    key_mob = index + 1;
  }

  // Never swap a mob into the player slot.
  if (index != PLAYER_MOB) {
    --num_mobs;
    if (index != num_mobs) {
      // Copy all fields (in SoA) from num_mobs => index.
      u8* dst = (u8*)mob_type + index;
      u8* src = (u8*)mob_type + num_mobs;
      for (u8 i = 0; i < MOB_FIELDS; ++i) {
        *dst = *src;
        src += MAX_MOBS;
        dst += MAX_MOBS;
      }
      mobmap[mob_pos[index]] = index + 1;
    }
  }
}

u8 animate_mobs(void) {
  u8 i, dosprite, visible, animdone;

  animdone = 1;
  dosprite = 0;

  // Loop through all mobs, update animations
  for (i = 0; i < num_mobs; ++i) {
    u8 pos = mob_pos[i];

    // TODO: We probably want to display mobs that are partially fogged; if the
    // start or end position is unfogged.
    u8 dotile = 0;
    if (visible = (!fogmap[pos] && mob_vis[i])) {
      if (--mob_anim_timer[i] == 0) {
        mob_anim_timer[i] = mob_anim_speed[i];
        if (++mob_anim_frame[i] == mob_type_anim_start[mob_type[i] + 1]) {
          mob_anim_frame[i] = mob_type_anim_start[mob_type[i]];
        }
        dotile = 1;
      }

      if (i + 1 == key_mob || mob_move_timer[i] || mob_flash[i]) {
        dosprite = 1;
      }

      if (dotile || dosprite) {
        u8 frame = mob_type_anim_frames[mob_anim_frame[i]];

        if (dosprite && mob_y[i] <= INV_TOP_Y()) {
          u8 prop = mob_flip[i] ? S_FLIPX : 0;
          *next_sprite++ = mob_y[i];
          *next_sprite++ = mob_x[i];
          if (mob_flash[i]) {
            *next_sprite++ = frame - TILE_MOB_FLASH_DIFF;
            *next_sprite++ = MOB_FLASH_HIT_PAL | prop;
            --mob_flash[i];
          } else {
            *next_sprite++ = frame;
            *next_sprite++ = prop | (i + 1 == key_mob ? MOB_FLASH_KEY_PAL : 0);
          }
        }

        if (mob_flip[i]) {
          frame += TILE_FLIP_DIFF;
        }

        mob_tile[i] = frame;
        dosprite = 0;
      }
    }

    if (mob_move_timer[i]) {
      mob_x[i] += mob_dx[i];
      mob_y[i] += mob_dy[i];

      --mob_move_timer[i];
      // Check for hop4 or pounce
      if ((mob_anim_state[i] & MOB_ANIM_STATE_HOP4_MASK) ==
          MOB_ANIM_STATE_HOP4) {
        mob_y[i] += hopy4[mob_move_timer[i]];
      }

      if (mob_move_timer[i] == 0) {
        switch (mob_anim_state[i]) {
          case MOB_ANIM_STATE_BUMP1:
            mob_anim_state[i] = MOB_ANIM_STATE_BUMP2;
            mob_move_timer[i] = BUMP_FRAMES;
            mob_dx[i] = -mob_dx[i];
            mob_dy[i] = -mob_dy[i];
            break;

          case MOB_ANIM_STATE_POUNCE:
            // bump player, if possible
            if (!ai_dobump(i)) {
              // Otherwise finish animation
              goto done;
            }
            break;

          done:
          case MOB_ANIM_STATE_WALK:
          case MOB_ANIM_STATE_BUMP2:
          case MOB_ANIM_STATE_HOP4:
            mob_anim_state[i] = MOB_ANIM_STATE_NONE;
            mob_dx[i] = mob_dy[i] = 0;
            if (visible) {
              dirty_tile(pos);
            }
            break;
        }
        // Reset animation speed
        mob_anim_speed[i] = mob_type_anim_speed[mob_type[i]];

        if (mob_trigger[i]) {
          mob_trigger[i] = 0;
          trigger_step(i);
        }
      } else {
        animdone = 0;
      }
    } else if (visible && dotile) {
      dirty_tile(pos);
    }
  }

  return animdone;
}

void adddeadmob(u8 index) {
  dmob_x[num_dead_mobs] = mob_x[index];
  dmob_y[num_dead_mobs] = mob_y[index];
  dmob_tile[num_dead_mobs] =
      mob_type_anim_frames[mob_anim_frame[index]] - TILE_MOB_FLASH_DIFF;
  dmob_prop[num_dead_mobs] =
      MOB_FLASH_HIT_PAL | (mob_flip[index] ? S_FLIPX : 0);
  dmob_timer[num_dead_mobs] = DEAD_MOB_FRAMES;
  ++num_dead_mobs;
}

void animate_deadmobs(void) {
  u8 i;
  for (i = 0; i < num_dead_mobs;) {
    if (--dmob_timer[i] == 0) {
      --num_dead_mobs;
      if (i != num_dead_mobs) {
        dmob_x[i] = dmob_x[num_dead_mobs];
        dmob_y[i] = dmob_y[num_dead_mobs];
        dmob_tile[i] = dmob_tile[num_dead_mobs];
        dmob_prop[i] = dmob_prop[num_dead_mobs];
        dmob_timer[i] = dmob_timer[num_dead_mobs];
      }
    } else if (dmob_y[i] <= INV_TOP_Y()) {
      *next_sprite++ = dmob_y[i];
      *next_sprite++ = dmob_x[i];
      *next_sprite++ = dmob_tile[i];
      *next_sprite++ = dmob_prop[i];
      ++i;
    }
  }
}

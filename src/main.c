#include <gb/gb.h>
#include <gb/gbdecompress.h>
#include <string.h>

#pragma bank 1

#include "main.h"

#include "ai.h"
#include "counter.h"
#include "gameplay.h"
#include "inventory.h"
#include "mob.h"
#include "palette.h"
#include "pickup.h"
#include "rand.h"
#include "sound.h"

#include "tilebg.h"
#include "tileshared.h"
#include "tilesprites.h"
#include "tiledead.h"

#define MAX_FLOATS 8   /* For now; we'll probably want more */
#define MAX_MSG_SPRITES 18
#define MAX_SPRS 32 /* Animation sprites for explosions, etc. */

// TODO: how big to make these arrays?
#define DIRTY_SIZE 128
#define DIRTY_CODE_SIZE 256
#define ANIM_TILES_SIZE 64

#define DEAD_X_OFFSET 4
#define DEAD_Y_OFFSET 2
#define DEAD_WIDTH 8
#define DEAD_HEIGHT 5

#define WIN_X_OFFSET 2
#define WIN_Y_OFFSET 2
#define WIN_WIDTH 12
#define WIN_HEIGHT 6

#define STATS_X_OFFSET 3
#define STATS_Y_OFFSET 9
#define STATS_WIDTH 9
#define STATS_HEIGHT 6

#define STATS_FLOOR_ADDR 0x992c
#define STATS_STEPS_ADDR 0x994c
#define STATS_KILLS_ADDR 0x996c

#define TILE_ANIM_FRAMES 8
#define TILE_ANIM_FRAME_DIFF 16

#define MSG_FRAMES 120
#define INV_TARGET_FRAMES 2
#define JOY_REPEAT_FIRST_WAIT_FRAMES 30
#define JOY_REPEAT_NEXT_WAIT_FRAMES 4

#define SIGHT_DIST_BLIND 1
#define SIGHT_DIST_DEFAULT 4

#define INV_TARGET_OFFSET 3

// Sprite tiles
#define TILE_ARROW_L 0x9
#define TILE_ARROW_R 0xa
#define TILE_ARROW_U 0xb
#define TILE_ARROW_D 0xc

void gameinit(void);
void begin_animate(void);
u8 animate(void);
void end_animate(void);
void hide_sprites(void);
void vbl_interrupt(void);

void update_sprites(void);

extern OAM_item_t shadow_OAM2[40];

u8 room_pos[MAX_ROOMS];
u8 room_w[MAX_ROOMS];
u8 room_h[MAX_ROOMS];
u8 room_avoid[MAX_ROOMS]; // ** only used during mapgen
u8 num_rooms;

u8 start_room;
u8 floor;
u8 startpos;

u8 num_cands;

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

u8 dirty[DIRTY_SIZE];
u8 dirty_code[DIRTY_CODE_SIZE];
u8 *dirty_ptr;

u8 anim_tiles[ANIM_TILES_SIZE];
u8 *anim_tile_ptr;
u8 anim_tile_timer; // timer for animating tiles (every TILE_ANIM_FRAMES frames)

OAM_item_t msg_sprites[MAX_MSG_SPRITES];
u8 msg_timer;

OAM_item_t float_sprites[MAX_FLOATS];
u8* next_float;

GameOverState gameover_state;
u8 gameover_timer;

u8 is_targeting;
Dir target_dir;

u8 joy, lastjoy, newjoy, repeatjoy;
u8 joy_repeat_count[8];
u8 joy_action; // The most recently pressed action button
u8 doupdatemap, dofadeout, doloadfloor, donextfloor, doblind, dosight;
u8 *next_sprite, *last_next_sprite;

u8 inv_target_timer;
u8 inv_target_frame;

Counter st_floor;
Counter st_steps;
Counter st_kills;
Counter st_recover;

void main(void) NONBANKED {
  // Initialize for title screen
  DISPLAY_OFF;
  clear_wram();
  pal_init();
  soundinit();
  enable_interrupts();

  SWITCH_ROM_MBC1(2);
  titlescreen();
  SWITCH_ROM_MBC1(1);

  // Initialize for gameplay
  init_win(0);
  LCDC_REG = LCDCF_ON | LCDCF_WINON | LCDCF_BGON | LCDCF_OBJON | LCDCF_WIN9C00;
  gb_decompress_bkg_data(0x80, tileshared_bin);
  gb_decompress_sprite_data(0, tilesprites_bin);
  add_VBL(vbl_interrupt);
  gameinit();
  doloadfloor = 1;

  while(1) {
    lastjoy = joy;
    joy = joypad();
    newjoy = (joy ^ lastjoy) & joy;
    repeatjoy = newjoy;

    if (joy) {
      u8 mask = 1, i = 0;
      while (mask) {
        if (newjoy & mask) {
          joy_repeat_count[i] = JOY_REPEAT_FIRST_WAIT_FRAMES;
        } else if (joy & mask) {
          if (--joy_repeat_count[i] == 0) {
            repeatjoy |= mask;
            joy_repeat_count[i] = JOY_REPEAT_NEXT_WAIT_FRAMES;
          }
        }

        if (repeatjoy & mask) {
          joy_action = mask;
        }

        mask <<= 1;
        ++i;
      }
    }

    // On floor 0, try to gather more entropy from player inputs.
    if (floor == 0 && newjoy) {
      xrnd_mix(joy);
      xrnd_mix(DIV_REG);
    }

    begin_animate();

    if (gameover_state != GAME_OVER_NONE) {
      if (gameover_state != GAME_OVER_WAIT) {
        end_animate();
        hide_sprites();
        pal_fadeout();
        IE_REG &= ~VBL_IFLAG;

        // Hide window
        move_win(23, 160);
        gb_decompress_bkg_data(0, tiledead_bin);
        init_bkg(0);
        if (gameover_state == GAME_OVER_WIN) {
          music_win();
          set_bkg_tiles(WIN_X_OFFSET, WIN_Y_OFFSET, WIN_WIDTH, WIN_HEIGHT,
                        win_map);
        } else {
          music_dead();
          set_bkg_tiles(DEAD_X_OFFSET, DEAD_Y_OFFSET, DEAD_WIDTH, DEAD_HEIGHT,
                        dead_map);
        }
        set_bkg_tiles(STATS_X_OFFSET, STATS_Y_OFFSET, STATS_WIDTH, STATS_HEIGHT,
                      stats_map);
        counter_out(&st_floor, STATS_FLOOR_ADDR);
        counter_out(&st_steps, STATS_STEPS_ADDR);
        counter_out(&st_kills, STATS_KILLS_ADDR);

        gameover_state = GAME_OVER_WAIT;
        IE_REG |= VBL_IFLAG;
        pal_fadein();
      } else {
        // Wait for keypress
        if (newjoy & J_A) {
          sfx(SFX_OK);
          gameover_state = GAME_OVER_NONE;
          doloadfloor = 1;
          pal_fadeout();

          // reset initial state
          music_main();
          gameinit();
        }
      }
    } else {
#if 0
      if (newjoy & J_START) { // XXX cheat
        dofadeout = donextfloor = doloadfloor = 1;
      }
#endif

      if (dofadeout) {
        dofadeout = 0;
        pal_fadeout();
      }
      if (donextfloor) {
        donextfloor = 0;
        ++floor;
        counter_inc(&st_floor);
        recover = 0;
      }
      if (doloadfloor) {
        doloadfloor = 0;
        inv_display_floor();
        hide_sprites();
        IE_REG &= ~VBL_IFLAG;
        SWITCH_ROM_MBC1(3);
        mapgen();
        SWITCH_ROM_MBC1(1);
        joy_action = 0;
        IE_REG |= VBL_IFLAG;
        end_animate();
        doupdatemap = 1;
        pal_fadein();
        begin_animate();
      }

      if (gameover_timer) {
        if (--gameover_timer == 0) {
          gameover_state = GAME_OVER_DEAD;
        }
      } else {
        do_turn();
      }

      update_sprites();

      while (1) {
        if (!animate()) break;

        if (pass_turn()) {
          if (!ai_run_tasks()) continue;
          // AI took a step, so do 1 round of animation
          animate();
        }
        break;
      }

      inv_animate();
      end_animate();
      pal_update();
    }
    wait_vbl_done();
  }
}

void gameinit(void) {
  dirty_ptr = dirty;
  dirty_code[0] = 0xc9;
  anim_tile_ptr = anim_tiles;
  anim_tile_timer = TILE_ANIM_FRAMES;

  // Reset scroll registers
  move_bkg(240, 0);
  // Reload bg tiles
  gb_decompress_bkg_data(0, tilebg_bin);
  init_bkg(0);

  // Reset player mob
  num_mobs = 0;
  addmob(MOB_TYPE_PLAYER, 0);
  inv_update_hp();

  turn = TURN_PLAYER;
  next_float = (u8*)float_sprites;

  // Set up inventory window
  move_win(23, 128);
  inv_init();
  inv_target_timer = INV_TARGET_FRAMES;

  floor = 0;
  counter_zero(&st_floor);
  counter_zero(&st_steps);
  counter_zero(&st_kills);
  joy_action = 0;
}

void begin_animate(void) {
  dirty_ptr = dirty;
  // Start mob sprites after floats. Double buffer the shadow_OAM so we don't
  // get partial updates when writing to it.
  next_sprite = (u8*)((_shadow_OAM_base << 8) ^ 0x100);
}

void end_animate(void) {
  // Hide the rest of the sprites
  while (next_sprite < last_next_sprite) {
    *next_sprite++ = 0;  // hide sprite
    next_sprite += 3;
  }
  last_next_sprite = next_sprite;

  // Flip the shadow_OAM buffer
  _shadow_OAM_base ^= 1;

  // Process dirty tiles
  u8* ptr = dirty;
  u16 last_addr = 0;
  u8 last_val = 0;

  dirty_code[0] = 0xc9;     // ret (changed to push af below)
  dirty_code[1] = 0xaf;     // xor a
  u8* code = dirty_code + 2;

  while (ptr != dirty_ptr) {
    u8 pos = *ptr++;
    u8 index, tile;
    u16 addr;

    // Prefer mobs, then pickups, and finally floor tiles
    if (fogmap[pos]) {
      tile = 0;
    } else {
      if ((index = mobmap[pos])) {
        --index;
        tile = mob_tile[index];
        if (!mob_move_timer[index]) {
          goto ok;
        }
      }

      if ((index = pickmap[pos])) {
        --index;
        tile = pick_tile[index];
        if (!pick_move_timer[index]) {
          goto ok;
        }
      }

      tile = tmap[pos];
    }

ok:
    addr = POS_TO_ADDR(pos);

    if ((addr >> 8) != (last_addr >> 8)) {
      *code++ = 0x21; // ld hl, addr
      *code++ = (u8)addr;
      *code++ = addr >> 8;
    } else {
      *code++ = 0x2e; // ld l, lo(addr)
      *code++ = (u8)addr;
    }
    last_addr = addr;
    if (tile != last_val) {
      if (tile == 0) {
        *code++ = 0xaf; // xor a
      } else {
        *code++ = 0x3e; // ld a, tile
        *code++ = tile;
      }
      last_val = tile;
    }
    *code++ = 0x77; // ld (hl), a

skip:
    // clear dirty map as we process tiles
    dirtymap[pos] = 0;
  }

  *code++ = 0xf1;       // pop af
  *code++ = 0xc9;       // ret
  dirty_code[0] = 0xf5; // push af
}

void hide_sprites(void) NONBANKED {
  memset(shadow_OAM, 0, 160);
  next_float = (u8*)float_sprites;
  msg_timer = 0;
  num_sprs = 0;
}

u8 animate(void) {
  u8 animdone = num_sprs == 0;

  // Loop through all animating tiles
  if (--anim_tile_timer == 0) {
    anim_tile_timer = TILE_ANIM_FRAMES;
    u8* ptr = anim_tiles;
    while (ptr != anim_tile_ptr) {
      u8 pos = *ptr++;
      tmap[pos] ^= TILE_ANIM_FRAME_DIFF;
      dirty_tile(pos);
    }
  }

  animdone &= animate_pickups();
  animdone &= animate_mobs();

  // Draw all dead mobs as sprites
  animate_deadmobs();

  return animdone;
}

void vbl_interrupt(void) NONBANKED {
  if (doupdatemap) {
    set_bkg_tiles(0, 0, MAP_WIDTH, MAP_HEIGHT, dtmap);
    doupdatemap = 0;
  }
  ((vfp)dirty_code)();
}

void addfloat(u8 pos, u8 tile) {
  if (next_float != (u8*)(float_sprites + MAX_FLOATS)) {
    *next_float++ = POS_TO_Y(pos);
    *next_float++ = POS_TO_X(pos);
    *next_float++ = tile;
    *next_float++ = FLOAT_FRAMES; // hijack prop for float time
  }
}

void showmsg(u8 index, u8 y) {
  u8 i, j;
  u8* p = (u8*)msg_sprites;
  for (j = 0; j < 2; ++j) {
    for (i = 0; i < 9; ++i) {
      *p++ = y;
      *p++ = 52 + (i << 3);
      *p++ = msg_tiles[index++];
      *p++ = 0;
    }
    y += 8;
  }
  msg_timer = MSG_FRAMES;
}

void update_sprites(void) {
  u8 i;
  // Display message sprites, if any
  if (msg_timer) {
    if (--msg_timer != 0 && MSG_REAPER_Y <= INV_TOP_Y()) {
      memcpy(next_sprite, (void*)msg_sprites, MAX_MSG_SPRITES * 4);
      next_sprite += MAX_MSG_SPRITES * 4;
    }
  }

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

  // Draw sprs
  for (i = 0; i < num_sprs;) {
    if (--spr_timer[i] == 0) {
      u8 dir, stun;
      switch (spr_type[i]) {
        case SPR_TYPE_SPIN:
          stun = 0;
          goto hit;

        case SPR_TYPE_BOLT:
          stun = 1;
          goto hit;

        hit:
          hitpos(spr_trigger_val[i], 1, stun);
          break;

        case SPR_TYPE_HOOK:
          dir = invdir[target_dir];
          goto push;

        case SPR_TYPE_PUSH:
          dir = target_dir;
          goto push;

        push: {
          u8 mob = spr_trigger_val[i];
          if (mob) {
            u8 pos = mob_pos[mob - 1];
            if (validmap[pos] & dirvalid[dir]) {
              u8 newpos = POS_DIR(pos, dir);
              if (!IS_WALL_OR_MOB(newpos)) {
                mobwalk(mob - 1, dir);
              } else {
                mobbump(mob - 1, dir);
              }
            }
            mob_stun[mob - 1] = mob_vis[mob - 1] = 1;
            sfx(SFX_MOB_PUSH);
          }
          break;
        }

        case SPR_TYPE_GRAPPLE:
          mobhop(PLAYER_MOB, spr_trigger_val[i]);
          break;
      }

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

  // Draw the targeting arrow
  if (is_targeting) {
    if (--inv_target_timer == 0) {
      inv_target_timer = INV_TARGET_FRAMES;
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
        u8 delta = INV_TARGET_OFFSET + pickbounce[inv_target_frame & 7];

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

u8 dropspot(u8 pos) {
  u8 i = 0, newpos;
  do {
    if (is_new_pos_valid(pos, drop_diff[i]) &&
        !(IS_SMARTMOB_POS(newpos = pos_result) || pickmap[newpos])) {
      return newpos;
    }
  } while (++i);
  return 0;
}

u8 addspr(u8 speed, u16 x, u16 y, u16 dx, u16 dy, u8 drag, u8 timer, u8 prop) {
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

void nop_saw_anim(u8 pos) {
  u8 index = sawmap[pos];
  pos = *--anim_tile_ptr;
  if (sawmap[pos]) { sawmap[pos] = index; }
  anim_tiles[index] = pos;
}


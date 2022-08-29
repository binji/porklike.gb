#include <gb/gb.h>
#include <gb/gbdecompress.h>

#include "main.h"

#include "ai.h"
#include "counter.h"
#include "float.h"
#include "gameplay.h"
#include "inventory.h"
#include "mob.h"
#include "palette.h"
#include "pickup.h"
#include "rand.h"
#include "sound.h"
#include "spr.h"
#include "sprite.h"
#include "targeting.h"

#pragma bank 1

#include "tilebg.h"
#include "tileshared.h"
#include "tilesprites.h"
#include "tiledead.h"

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

#define JOY_REPEAT_FIRST_WAIT_FRAMES 30
#define JOY_REPEAT_NEXT_WAIT_FRAMES 4

void gameinit(void);
void begin_animate(void);
u8 animate(void);
void end_animate(void);
void vbl_interrupt(void);

u8 floor;

u8 dirty[DIRTY_SIZE];
u8 dirty_code[DIRTY_CODE_SIZE];
u8 *dirty_ptr;

u8 anim_tiles[ANIM_TILES_SIZE];
u8 *anim_tile_ptr;
u8 anim_tile_timer; // timer for animating tiles (every TILE_ANIM_FRAMES frames)

GameOverState gameover_state;
u8 gameover_timer;

u8 joy, lastjoy, newjoy, repeatjoy;
u8 joy_repeat_count[8];
u8 joy_action; // The most recently pressed action button
u8 doupdatemap, dofadeout, doloadfloor, donextfloor, doblind, dosight;

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
        sprite_hide();
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
        sprite_hide();
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

      sprite_update();

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
  float_hide();

  // Set up inventory window
  move_win(23, 128);
  inv_init();
  targeting_init();

  floor = 0;
  counter_zero(&st_floor);
  counter_zero(&st_steps);
  counter_zero(&st_kills);
  joy_action = 0;
}

void begin_animate(void) {
  dirty_ptr = dirty;
  sprite_animate_begin();
}

void end_animate(void) {
  sprite_animate_end();

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

void nop_saw_anim(u8 pos) {
  u8 index = sawmap[pos];
  pos = *--anim_tile_ptr;
  if (sawmap[pos]) { sawmap[pos] = index; }
  anim_tiles[index] = pos;
}


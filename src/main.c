#include <gb/gb.h>
#include <gb/gbdecompress.h>
#include <string.h>

#pragma bank 1

#include "main.h"
#include "rand.h"
#include "counter.h"
#include "mob.h"
#include "pickup.h"

#include "tilebg.h"
#include "tileshared.h"
#include "tilesprites.h"
#include "tiledead.h"

#define MAX_FLOATS 8   /* For now; we'll probably want more */
#define MAX_MSG_SPRITES 18
#define MAX_EQUIPS 4
#define MAX_SPRS 32 /* Animation sprites for explosions, etc. */

#define MSG_REAPER 0
#define MSG_REAPER_Y 83
#define MSG_KEY 18
#define MSG_KEY_Y 80
#define MSG_LEN 18

// TODO: how big to make these arrays?
#define DIRTY_SIZE 128
#define DIRTY_CODE_SIZE 256
#define ANIM_TILES_SIZE 64

#define INV_WIDTH 16
#define INV_HEIGHT 13
#define INV_HP_ADDR 0x9c22
#define INV_KEYS_ADDR 0x9c25
#define INV_FLOOR_ADDR 0x9c27
#define INV_FLOOR_NUM_ADDR 0x9c2d
#define INV_EQUIP_ADDR 0x9c63
#define INV_DESC_ADDR 0x9d01
#define INV_SELECT_X_OFFSET 32
#define INV_SELECT_Y_OFFSET 40
#define INV_FLOOR_LEN 5          /* FLOOR */
#define INV_FLOOR_3_SPACES_LEN 8 /* "FLOOR   " */
#define INV_BLIND_LEN 5          /* BLIND */
#define INV_FLOOR_OFFSET 23      /* offset into inventory_map */
#define INV_ROW_LEN 14
#define INV_BLANK_ROW_OFFSET 49 /* offset into inventory_map */

#define GAMEOVER_FRAMES 70

#define GAMEOVER_X_OFFSET 3
#define GAMEOVER_Y_OFFSET 2
#define GAMEOVER_WIDTH 9
#define GAMEOVER_HEIGHT 13

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
#define TILE_VOID_EXIT_DIFF 0x35
#define TILE_VOID_OPEN_DIFF 3
#define TILE_PLAYER_HIT_DMG_DIFF 0x5f

#define FADE_FRAMES 8
#define INV_ANIM_FRAMES 25

#define FLOAT_FRAMES 70
#define OBJ1_PAL_FRAMES 8
#define MOB_FLASH_FRAMES 20
#define MSG_FRAMES 120
#define INV_SELECT_FRAMES 8
#define INV_TARGET_FRAMES 2
#define JOY_REPEAT_FIRST_WAIT_FRAMES 30
#define JOY_REPEAT_NEXT_WAIT_FRAMES 8

#define SPIN_SPR_FRAMES 12
#define SPIN_SPR_ANIM_SPEED 3

#define QUEEN_CHARGE_MOVES 2
#define AI_COOL_MOVES 8
#define RECOVER_MOVES 30

#define REAPER_AWAKENS_STEPS 100

#define SIGHT_DIST_BLIND 1
#define SIGHT_DIST_DEFAULT 4

#define INV_TARGET_OFFSET 3
#define FLOAT_BLIND_X_OFFSET 0xfb

#define PICKUP_CHARGES 2

#define IS_MOB(pos) (mobmap[pos])
#define IS_WALL_OR_MOB(pos) (IS_WALL_POS(pos) || mobmap[pos])
#define IS_UNSPECIAL_WALL_TILE(tile)                                           \
  ((flags_bin[tile] & 0b00000011) == 0b00000001)

// Sprite tiles
#define TILE_SHOT 0x1
#define TILE_ROPE_TIP_V 0x2
#define TILE_ROPE_TIP_H 0x3
#define TILE_ROPE_TAIL_H 0x4
#define TILE_ROPE_TAIL_V 0xd
#define TILE_ARROW_L 0x9
#define TILE_ARROW_R 0xa
#define TILE_ARROW_U 0xb
#define TILE_ARROW_D 0xc
#define TILE_SPIN 0x10
#define TILE_BLIND 0x37
#define TILE_BOOM 0x05

// Shared tiles
#define TILE_0 0xe5

#define FLOAT_FOUND 0xe
#define FLOAT_LOST 0xf

typedef enum GameOverState {
  GAME_OVER_NONE,
  GAME_OVER_DEAD,
  GAME_OVER_WIN,
  GAME_OVER_WAIT,
} GameOverState;

typedef enum Turn {
  TURN_PLAYER,
  TURN_PLAYER_MOVED,
  TURN_AI,
  TURN_WEEDS,
} Turn;

typedef enum SprType {
  SPR_TYPE_NONE,
  SPR_TYPE_BOOM,
  SPR_TYPE_BOLT,
  SPR_TYPE_PUSH,
  SPR_TYPE_GRAPPLE,
  SPR_TYPE_HOOK,
  SPR_TYPE_SPIN,
} SprType;

void gameinit(void);
void do_turn(void);
void pass_turn(void);
void begin_animate(void);
void move_player(void);
void use_pickup(void);
u8 shoot(u8 pos, u8 hit, u8 tile, u8 prop);
u8 shoot_dist(u8 pos, u8 hit);
u8 rope(u8 from, u8 to);
void hitmob(u8 index, u8 dmg);
void hitpos(u8 pos, u8 dmg, u8 stun);
void update_wall_face(u8 pos);
void ai_run_tasks(void);
u8 ai_run_mob_task(u8 index);
u8 ai_tcheck(u8 index);
u8 ai_getnextstep(u8 index);
u8 ai_getnextstep_rev(u8 index);
void blind(void);
void calcdist_ai(u8 from, u8 to);
void animate(void);
void end_animate(void);
void hide_sprites(void);
void update_tile(u8 pos, u8 tile);
void unfog_tile(u8 pos);
void vbl_interrupt(void);

void addfloat(u8 pos, u8 tile);
void update_sprites(void);
void showmsg(u8 index, u8 y);

void inv_animate(void);

void update_obj1_pal(void);
void fadeout(void);
void fadein(void);

u8 is_valid(u8 pos, u8 diff) __preserves_regs(b, c);

u8 addspr(u8 speed, u16 x, u16 y, u16 dx, u16 dy, u8 drag, u8 timer, u8 prop);

void nop_saw_anim(u8 pos);

extern OAM_item_t shadow_OAM2[40];
extern u8 mobsightmap[256];

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

Turn turn;
u8 dopassturn, doai;
u8 noturn;
GameOverState gameover_state;
u8 gameover_timer;

u8 is_targeting;
Dir target_dir;

u8 joy, lastjoy, newjoy, repeatjoy;
u8 joy_repeat_count[8];
u8 joy_action; // The most recently pressed action button
u8 doupdatemap, dofadeout, doloadfloor, donextfloor, doblind, dosight;
u8 *next_sprite, *last_next_sprite;

u8 inv_anim_up;
u8 inv_anim_timer;
u8 inv_select;
u8 inv_select_timer;
u8 inv_select_frame;
u8 inv_target_timer;
u8 inv_target_frame;
u8 inv_msg_update;
PickupType inv_selected_pick;

u8 equip_type[MAX_EQUIPS];
u8 equip_charge[MAX_EQUIPS];
u8 num_keys;
u8 recover; // how long until recovering from blind
u16 steps;

Counter st_floor;
Counter st_steps;
Counter st_kills;
Counter st_recover;

u8 obj_pal1_timer, obj_pal1_index;

void main(void) NONBANKED {
  // Initialize for title screen
  DISPLAY_OFF;
  clear_wram();
  BGP_REG = OBP0_REG = OBP1_REG = fadepal[0];
  obj_pal1_timer = OBJ1_PAL_FRAMES;
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
        fadeout();
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
        fadein();
      } else {
        // Wait for keypress
        if (newjoy & J_A) {
          sfx(SFX_OK);
          gameover_state = GAME_OVER_NONE;
          doloadfloor = 1;
          fadeout();

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
        fadeout();
      }
      if (donextfloor) {
        donextfloor = 0;
        ++floor;
        counter_inc(&st_floor);

        // Set back to FLOOR #
        vram_copy(INV_FLOOR_ADDR, inventory_map + INV_FLOOR_OFFSET,
                  INV_FLOOR_3_SPACES_LEN);
        recover = 0;
      }
      if (doloadfloor) {
        doloadfloor = 0;
        counter_out(&st_floor, INV_FLOOR_NUM_ADDR);
        hide_sprites();
        IE_REG &= ~VBL_IFLAG;
        SWITCH_ROM_MBC1(3);
        mapgen();
        SWITCH_ROM_MBC1(1);
        joy_action = 0;
        IE_REG |= VBL_IFLAG;
        end_animate();
        doupdatemap = 1;
        fadein();
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

      do {
        animate();
        if (dopassturn) {
          dopassturn = 0;
          pass_turn();
          if (doai) {
            doai = 0;
            ai_run_tasks();
            if (!dopassturn) {
              // AI took a step, so do 1 round of animation
              animate();
            }
          }
        }
      } while (dopassturn);

      inv_animate();
      end_animate();
      update_obj1_pal();
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
  set_vram_byte((u8 *)INV_HP_ADDR, TILE_0 + mob_hp[PLAYER_MOB]);

  turn = TURN_PLAYER;
  next_float = (u8*)float_sprites;

  // Set up inventory window
  move_win(23, 128);
  inv_select_timer = INV_SELECT_FRAMES;
  inv_target_timer = INV_TARGET_FRAMES;
  inv_selected_pick = PICKUP_TYPE_NONE;
  inv_msg_update = 1;
  memset(equip_type, PICKUP_TYPE_NONE, sizeof(equip_type));
  set_win_tiles(0, 0, INV_WIDTH, INV_HEIGHT, inventory_map);

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

void do_turn(void) {
  if (inv_anim_up) {
    if (!inv_anim_timer) {
      if (joy_action & (J_UP | J_DOWN)) {
        if (joy_action & J_UP) {
          inv_select += 3;
        } else if (joy_action & J_DOWN) {
          ++inv_select;
        }
        inv_select &= 3;
        inv_msg_update = 1;
        inv_selected_pick = equip_type[inv_select];
        sfx(SFX_SELECT);
      }
      if (newjoy & J_B) {
        inv_anim_timer = INV_ANIM_FRAMES;
        inv_anim_up = 0;
        sfx(SFX_BACK);
      } else if (joy_action & J_A) {
        if (inv_selected_pick == PICKUP_TYPE_NONE) {
          sfx(SFX_OOPS);
        } else {
          inv_anim_timer = INV_ANIM_FRAMES;
          inv_anim_up = 0;
          is_targeting = 1;
          sfx(SFX_OK);
        }
      } else if (joy == (J_LEFT | J_START)) {
        // Display level seed instead of the floor
        static const u8 hextile[] = {
            0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec,
            0xed, 0xee, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0,
        };

        set_vram_byte((u8 *)(INV_FLOOR_ADDR),
                      hextile[(floor_seed >> 12) & 0xf]);
        set_vram_byte((u8 *)(INV_FLOOR_ADDR + 1),
                      hextile[(floor_seed >> 8) & 0xf]);
        set_vram_byte((u8 *)(INV_FLOOR_ADDR + 2),
                      hextile[(floor_seed >> 4) & 0xf]);
        set_vram_byte((u8 *)(INV_FLOOR_ADDR + 3), hextile[floor_seed & 0xf]);
        set_vram_byte((u8 *)(INV_FLOOR_ADDR + 4), TILE_0 + dogate);
        set_vram_byte((u8 *)(INV_FLOOR_ADDR + 5), 0);
        set_vram_byte((u8 *)(INV_FLOOR_ADDR + 6), 0);
        set_vram_byte((u8 *)(INV_FLOOR_ADDR + 7), 0);
      }
      joy_action = 0;
    }
  } else if (turn == TURN_PLAYER) {
    move_player();
  }
}

void pass_turn(void) {
  if (dosight) {
    dosight = 0;
    sight();
  }

  switch (turn) {
  case TURN_PLAYER_MOVED:
    if (noturn) {
      turn = TURN_PLAYER;
      noturn = 0;
    } else {
      turn = TURN_AI;
      doai = 1;
    }
    break;

  case TURN_AI:
    turn = TURN_WEEDS;
    doai = 1;
    break;

  case TURN_WEEDS:
    turn = TURN_PLAYER;
    if (floor && steps == REAPER_AWAKENS_STEPS) {
      ++steps; // Make sure to only spawn reaper once per floor
      addmob(MOB_TYPE_REAPER, dropspot(startpos));
      mob_active[num_mobs - 1] = 1;
      showmsg(MSG_REAPER, MSG_REAPER_Y);
      sfx(SFX_REAPER);
    }
    if (doblind) {
      // Update floor to display recover count instead
      vram_copy(INV_FLOOR_ADDR, blind_map, INV_BLIND_LEN);
      counter_thirty(&st_recover);
      counter_out(&st_recover, INV_FLOOR_NUM_ADDR);

      // Display float
      u8 i, x, y;
      x = POS_TO_X(mob_pos[PLAYER_MOB]) + FLOAT_BLIND_X_OFFSET;
      y = POS_TO_Y(mob_pos[PLAYER_MOB]);
      for (i = 0; i < 3; ++i) {
        *next_float++ = y;
        *next_float++ = x;
        *next_float++ = TILE_BLIND + i;
        *next_float++ = FLOAT_FRAMES;  // hijack prop for float timer
        x += 8;
      }

      doblind = 0;
    } else if (recover) {
      if (--recover == 0) {
        // Set back to FLOOR #
        vram_copy(INV_FLOOR_ADDR, inventory_map + INV_FLOOR_OFFSET,
                  INV_FLOOR_LEN);
        counter_out(&st_floor, INV_FLOOR_NUM_ADDR);
        dosight = 1;
      } else {
        // Decrement recover counter
        counter_dec(&st_recover);
        // Blank out the right tile, in case of 10 -> 9
        set_vram_byte((u8 *)(INV_FLOOR_NUM_ADDR + 1), 0);
        counter_out(&st_recover, INV_FLOOR_NUM_ADDR);
      }
    }
    break;
  }
}

void move_player(void) {
  u8 dir, tile;

  if (mob_move_timer[PLAYER_MOB] == 0 && !inv_anim_timer) {
    if (joy_action & (J_LEFT | J_RIGHT | J_UP | J_DOWN)) {
      if (joy_action & J_LEFT) {
        dir = DIR_LEFT;
      } else if (joy_action & J_RIGHT) {
        dir = DIR_RIGHT;
      } else if (joy_action & J_UP) {
        dir = DIR_UP;
      } else {
        dir = DIR_DOWN;
      }

      if (is_targeting) {
        if (dir != target_dir) {
          target_dir = dir;
          mobdir(PLAYER_MOB, dir);
          mob_anim_timer[PLAYER_MOB] = 1;  // Update the player's direction
          sfx(SFX_SELECT);
        }
      } else {
        u8 pos = mob_pos[PLAYER_MOB];
        if (validmap[pos] & dirvalid[dir]) {
          u8 newpos = POS_DIR(pos, dir);
          if (IS_MOB(newpos)) {
            mobbump(PLAYER_MOB, dir);
            if (mob_type[mobmap[newpos] - 1] == MOB_TYPE_SCORPION &&
                XRND_50_PERCENT()) {
              blind();
            }
            hitmob(mobmap[newpos] - 1, 1);
            goto done;
          } else {
            tile = tmap[newpos];
            if (IS_WALL_TILE(tile)) {
              if (IS_SPECIAL_TILE(tile)) {
                if (tile == TILE_GATE) {
                  if (num_keys) {
                    --num_keys;

                    update_tile(newpos, TILE_EMPTY);
                    set_vram_byte((u8 *)INV_KEYS_ADDR, TILE_0 + num_keys);
                    dosight = 1;
                    sfx(SFX_BUMP_TILE);
                    goto dobump;
                  } else {
                    showmsg(MSG_KEY, MSG_KEY_Y);
                    sfx(SFX_OK);
                  }
                } else if (tile == TILE_KIELBASA) {
                  gameover_state = GAME_OVER_WIN;
                } else if (tile == TILE_VOID_BUTTON_U ||
                           tile == TILE_VOID_BUTTON_L ||
                           tile == TILE_VOID_BUTTON_R ||
                           tile == TILE_VOID_BUTTON_D) {
                  update_tile(newpos, tile + TILE_VOID_OPEN_DIFF);
                  update_wall_face(newpos);
                  dosight = 1;
                  sfx(SFX_BUMP_TILE);
                  goto dobump;
                }
              }
#if 0
              // XXX: bump to destroy walls for testing
              else {
                update_tile(newpos, TILE_EMPTY);
                goto dobump;
              }
#endif
              // fallthrough to bump below...
            } else {
              mobwalk(PLAYER_MOB, dir);
              ++steps;
              counter_inc(&st_steps);
              sfx(SFX_PLAYER_STEP);
              goto done;
            }
          }
        }

        // Bump w/o taking a turn
        noturn = 1;
      dobump:
        mob_trigger[PLAYER_MOB] = !noturn;
        mobbump(PLAYER_MOB, dir);
      done:
        turn = TURN_PLAYER_MOVED;
      }
    } else if (is_targeting) {
      if (newjoy & J_B) {
        // Back out of targeting
        inv_anim_timer = INV_ANIM_FRAMES;
        inv_anim_up = 1;
        is_targeting = 0;
        sfx(SFX_BACK);
      } else if (newjoy & J_A) {
        use_pickup();
        turn = TURN_PLAYER_MOVED;
        sfx(SFX_OK);
      }
    } else if (joy_action & J_A) {
      // Open inventory
      inv_anim_timer = INV_ANIM_FRAMES;
      inv_anim_up = 1;
      sfx(SFX_OK);
    }
    joy_action = 0;
  }
}

void use_pickup(void) {
  u8 pos = mob_pos[PLAYER_MOB];
  u8 valid1 = validmap[pos] & dirvalid[target_dir];
  u8 pos1 = POS_DIR(pos, target_dir);
  u8 valid2 = valid1 && (validmap[pos1] & dirvalid[target_dir]);
  u8 pos2 = POS_DIR(pos1, target_dir);
  u8 validb = validmap[pos] & dirvalid[invdir[target_dir]];
  u8 posb = POS_DIR(pos, invdir[target_dir]);

  u8 valid, hit, land;
  land = hit = pos;
  do {
    valid = validmap[hit] & dirvalid[target_dir];
    land = hit;
    if (valid) {
      hit = POS_DIR(hit, target_dir);
    }
  } while(valid && !IS_WALL_OR_MOB(hit));

  u8 mob1 = mobmap[pos1];
  u8 mob2 = mobmap[pos2];
  u8 mobh = mobmap[hit];

  u8 spr;
  switch (inv_selected_pick) {
    case PICKUP_TYPE_JUMP:
      if (valid2 && !IS_WALL_OR_MOB(pos2)) {
        if (mob1) {
          mob_stun[mob1 - 1] = 1;
        }
        mobhop(PLAYER_MOB, pos2);
        sfx(SFX_USE_EQUIP);
      } else {
        sfx(SFX_FAIL);
      }
      break;

    case PICKUP_TYPE_BOLT:
      spr = shoot(pos, hit, TILE_SHOT, 0);
      spr_type[spr] = SPR_TYPE_BOLT;
      spr_trigger_val[spr] = hit;
      sfx(SFX_USE_EQUIP);
      break;

    case PICKUP_TYPE_PUSH:
      spr = shoot(pos, hit, TILE_SHOT, 0);
      spr_type[spr] = SPR_TYPE_PUSH;
      spr_trigger_val[spr] = mobh;
      sfx(SFX_USE_EQUIP);
      break;

    case PICKUP_TYPE_GRAPPLE:
      spr = rope(pos, land);
      spr_type[spr] = SPR_TYPE_GRAPPLE;
      spr_trigger_val[spr] = land;
      sfx(SFX_USE_EQUIP);
      break;

    case PICKUP_TYPE_SPEAR:
      if (valid1) {
        if (valid2) {
          spr = rope(pos, pos2);
          hitpos(pos2, 1, 0);
        } else {
          spr = rope(pos, pos1);
        }
        spr_type[spr] = SPR_TYPE_NONE;
        hitpos(pos1, 1, 0);
      }
      mobbump(PLAYER_MOB, target_dir);
      sfx(SFX_SPEAR);
      break;

    case PICKUP_TYPE_SMASH:
      mobbump(PLAYER_MOB, target_dir);
      if (valid1) {
        hitpos(pos1, 2, 0);
        if (IS_BREAKABLE_WALL_POS(pos1)) {
          update_tile(pos1, dirt_tiles[xrnd() & 3]);
          update_wall_face(pos1);
          sfx(SFX_DESTROY_WALL);
        }
      }
      break;

    case PICKUP_TYPE_HOOK:
      spr = rope(pos, hit);
      spr_type[spr] = SPR_TYPE_HOOK;
      spr_trigger_val[spr] = mobh;
      sfx(SFX_USE_EQUIP);
      break;

    case PICKUP_TYPE_SPIN: {
      u16 x = POS_TO_X(pos) << 8;
      u16 y = POS_TO_Y(pos) << 8;
      u8 valid = validmap[pos];
      u8 dir;
      for (dir = 0; dir < 4; ++dir) {
        if (valid & dirvalid[dir]) {
          spr = addspr(SPIN_SPR_ANIM_SPEED + dir, x, y, dirx2[dir] << 8,
                       diry2[dir] << 8, 1, SPIN_SPR_FRAMES + (dir << 2), 0);
          spr_type[spr] = SPR_TYPE_SPIN;
          spr_anim_frame[spr] = TILE_SPIN;
          spr_trigger_val[spr] = POS_DIR(pos, dir);
        }
      }
      mobhop(PLAYER_MOB, pos);
      sfx(SFX_SPIN);
      break;
    }

    case PICKUP_TYPE_SUPLEX:
      if (valid1 && validb && mob1 && !IS_WALL_OR_MOB(posb)) {
        mobhop(mob1 - 1, posb);
        mob_stun[mob1 - 1] = 1;
        mob_trigger[mob1 - 1] = 1;
        mobbump(PLAYER_MOB, target_dir);
        sfx(SFX_USE_EQUIP);
      } else {
        sfx(SFX_FAIL);
      }
      break;

    case PICKUP_TYPE_SLAP:
      if (validb) {
        if (IS_WALL_OR_MOB(posb)) {
          mobbump(PLAYER_MOB, invdir[target_dir]);
        } else {
          mobhop(PLAYER_MOB, posb);
          sfx(SFX_USE_EQUIP);
        }
      }
      if (valid1) {
        hitpos(pos1, 1, 1);
      }
      break;
  }

  u16 equip_addr = INV_EQUIP_ADDR - 2 + (inv_select << 5);
  if (--equip_charge[inv_select] == 0) {
    equip_type[inv_select] = PICKUP_TYPE_NONE;
    inv_selected_pick = PICKUP_TYPE_NONE;
    inv_msg_update = 1;

    // Clear equip display
    vram_copy(equip_addr, inventory_map + INV_BLANK_ROW_OFFSET, INV_ROW_LEN);
  } else {
    // Update charge count display
    set_vram_byte((u8 *)(equip_addr + pick_type_name_len[inv_selected_pick]),
                  TILE_0 + equip_charge[inv_select]);
  }

  mob_trigger[PLAYER_MOB] = 1;
  dosight = 1;
  is_targeting = 0;
}

u8 shoot(u8 pos, u8 hit, u8 tile, u8 prop) {
  u8 spr = addspr(255, POS_TO_X(pos) << 8, POS_TO_Y(pos) << 8,
                  dirx3[target_dir] << 8, diry3[target_dir] << 8, 0,
                  n_over_3[shoot_dist(pos, hit)], prop);
  spr_anim_frame[spr] = tile;
  return spr;
}

u8 shoot_dist(u8 pos, u8 hit) {
  u8 dist = (target_dir == DIR_LEFT)    ? pos - hit
            : (target_dir == DIR_RIGHT) ? hit - pos
            : (target_dir == DIR_UP)    ? (pos - hit) >> 4
                                        : (hit - pos) >> 4;
  return dist;
}

u8 rope(u8 from, u8 to) {
  u8 tip, tail, prop;
  prop = 0;
  if (from == to) { return 0; }

  u8 pos = from;
  u16 x = (POS_TO_X(pos) + dirx4[target_dir]) << 8;
  u16 y = (POS_TO_Y(pos) + diry4[target_dir]) << 8;
  u16 dx = 0;
  u16 dy = 0;
  u16 ddx, ddy;
  u8 dist = shoot_dist(from, to);
  u8 timer = n_over_3[dist];
  u16 delta = three_over_n[dist];

  if (target_dir == DIR_LEFT || target_dir == DIR_RIGHT) {
    tip = TILE_ROPE_TIP_H;
    tail = TILE_ROPE_TAIL_H;
    if (target_dir == DIR_LEFT) {
      ddx = -delta;
    } else {
      prop = S_FLIPX;
      ddx = delta;
    }
    ddy = 0;
  } else {
    tip = TILE_ROPE_TIP_V;
    tail = TILE_ROPE_TAIL_V;
    if (target_dir == DIR_UP) {
      ddy = -delta;
    } else {
      prop = S_FLIPY;
      ddy = delta;
    }
    ddx = 0;
  }

  do {
    u8 spr = addspr(255, x, y, dx, dy, 0, timer, prop);
    spr_type[spr] = SPR_TYPE_NONE;
    spr_anim_frame[spr] = tail;
    dx += ddx;
    dy += ddy;
    pos = POS_DIR(pos, target_dir);
  } while(pos != to);

  return shoot(from, to, tip, prop);
}

void hitmob(u8 index, u8 dmg) {
  u8 mob, mtype, percent, tile, slime, pos, valid, i;
  u16 x, y;
  pos = mob_pos[index];
  mtype = mob_type[index];

  if (mob_type_object[mtype]) {
    u8 sound = SFX_OPEN_OBJECT;

    if (mtype == MOB_TYPE_CHEST || mtype == MOB_TYPE_HEART_CHEST) {
      percent = 100;
      tile = TILE_CHEST_OPEN;
      slime = 0;
      sound = SFX_BUMP_TILE;
    } else if (mtype == MOB_TYPE_BOMB) {
      percent = 100;
      tile = TILE_BOMB_EXPLODED;
      slime = 0;
      sound = SFX_BOOM;
    } else {
      percent = 20;
      if (!(validmap[pos] & VALID_U) || IS_BREAKABLE_WALL_POS(POS_U(pos))) {
        tile = TILE_WALL_FACE_RUBBLE;
      } else {
        tile = dirt_tiles[xrnd() & 3];
      }
      slime = 1;
    }

    delmob(index);
    if (!IS_SPECIAL_POS(pos)) {
      update_tile(pos, tile);
    }

    if (randint(100) < percent) {
      if (slime && XRND_20_PERCENT()) {
        mob = num_mobs;
        addmob(MOB_TYPE_SLIME, pos);
        mobhopnew(mob, dropspot(pos));
        sound = SFX_OOPS;
      } else if (mtype == MOB_TYPE_HEART_CHEST && XRND_20_PERCENT()) {
        droppick(PICKUP_TYPE_HEART, pos);
      } else {
        droppick_rnd(pos);
      }
    }

    // Explode bomb after the mob is deleted (to prevent infinite recursion)
    if (mtype == MOB_TYPE_BOMB) {
      // Destroy the surrounding tiles
      valid = validmap[pos];
      u8 dir = 7;
      do {
        if (valid & dirvalid[dir]) {
          u8 newpos = POS_DIR(pos, dir);
          u8 tile = tmap[newpos];
          if (IS_WALL_FACE_TILE(tile)) {
            tile = TILE_WALL_FACE_RUBBLE;
          } else if (IS_UNSPECIAL_WALL_TILE(tile)) {
            tile = dirt_tiles[xrnd() & 3];
          } else {
            goto noupdate;
          }
          update_tile(newpos, tile);
        noupdate:
          hitpos(newpos, dmg, 0);
        }
      } while (dir--);
      update_tile(pos, TILE_BOMB_EXPLODED);

      x = POS_TO_X(pos) << 8;
      y = POS_TO_Y(pos) << 8;

      u8 spr;
      for (i = 0; i < NUM_BOOM_SPRS; ++i) {
        spr =
            addspr(boom_spr_anim_speed[i], x + boom_spr_x[i], y + boom_spr_y[i],
                   boom_spr_dx[i], boom_spr_dy[i], 1, boom_spr_speed[i], 0);
        spr_type[spr] = SPR_TYPE_BOOM;
        spr_anim_frame[spr] = TILE_BOOM;
      }
    }

    sfx(sound);
  } else {
    addfloat(pos, float_dmg[dmg] +
                      (index == PLAYER_MOB ? TILE_PLAYER_HIT_DMG_DIFF : 0));

    if (mob_hp[index] <= dmg) {
      adddeadmob(index);
      // drop key, if any.
      // NOTE: This code is subtle; delmob() will update the
      // key_mob, and droppick() must be called after delmob() so the pickup
      // can be dropped where the mob was.
      if (index + 1 == key_mob) {
        delmob(index);
        droppick(PICKUP_TYPE_KEY, pos);
      } else {
        delmob(index);
      }

      dirty_tile(pos);

      if (index == PLAYER_MOB) {
        set_vram_byte((u8 *)INV_HP_ADDR, TILE_0);
        gameover_timer = GAMEOVER_FRAMES;
      } else {
        // Killing a reaper restores the your health to 5 and drops a free key
        if (mtype == MOB_TYPE_REAPER) {
          if (mob_hp[PLAYER_MOB] < 5) {
            mob_hp[PLAYER_MOB] = 5;
            set_vram_byte((u8 *)INV_HP_ADDR, (u8)(TILE_0 + 5));
          }
          droppick(PICKUP_TYPE_KEY, pos);
          sfx(SFX_HEART);
        } else {
          sfx(SFX_HIT_MOB);
        }
        counter_inc(&st_kills);
      }
    } else {
      mob_flash[index] = MOB_FLASH_FRAMES;
      mob_hp[index] -= dmg;
      mob_vis[index] = 1;
      if (index == PLAYER_MOB) {
        set_vram_byte((u8 *)INV_HP_ADDR, TILE_0 + mob_hp[PLAYER_MOB]);
        sfx(SFX_HIT_PLAYER);
      } else {
        sfx(SFX_HIT_MOB);
      }
    }
  }
}

void hitpos(u8 pos, u8 dmg, u8 stun) {
  u8 tile, mob;
  tile = tmap[pos];
  if ((mob = mobmap[pos])) {
    // Stun first, since hitmob may delete the mob.
    if (stun) {
      mob_stun[mob - 1] = 1;
    }
    hitmob(mob - 1, dmg); // XXX maybe recursive...
  }
  if ((tmap[pos] & TILE_SAW_MASK) == TILE_SAW) {
    tile = TILE_SAW_BROKEN;
    nop_saw_anim(pos);
    sfx(SFX_DESTROY_WALL);
  } else if (IS_CRACKED_WALL_TILE(tile)) {
    tile = dirt_tiles[xrnd() & 3];
    update_wall_face(pos);
    sfx(SFX_DESTROY_WALL);
  } else {
    goto noupdate;
  }
  update_tile(pos, tile);
noupdate:
  dosight = 1;
}

void update_wall_face(u8 pos) {
  if (validmap[pos] & VALID_D) {
    pos = POS_D(pos);
    u8 tile = tmap[pos];
    if (tile == TILE_WALL_FACE || tile == TILE_WALL_FACE_CRACKED) {
      tile = TILE_EMPTY;
    } else if (tile == TILE_WALL_FACE_RUBBLE) {
      tile = dirt_tiles[xrnd() & 3];
    } else if (tile == TILE_WALL_FACE_PLANT) {
      tile = TILE_PLANT1;
    }
    update_tile(pos, tile);
  }
}

void ai_run_tasks(void) {
  dopassturn = 1;
  u8 i;
  for (i = 0; i < num_mobs; ++i) {
    if ((mob_type[i] == MOB_TYPE_WEED) == (turn == TURN_WEEDS)) {
      dopassturn &= !ai_run_mob_task(i);
    }
  }
}

u8 ai_run_mob_task(u8 index) {
  u8 pos, mob, dir, valid;
  if (mob_stun[index]) {
    mob_stun[index] = 0;
  } else {
    pos = mob_pos[index];
    switch (mob_task[index]) {
      case MOB_AI_WAIT:
        if (mobsightmap[pos]) {
          addfloat(pos, FLOAT_FOUND);
          mob_task[index] = mob_type_ai_active[mob_type[index]];
          mob_target_pos[index] = mob_pos[PLAYER_MOB];
          mob_ai_cool[index] = 0;
          mob_active[index] = 1;
          joy_action = 0;
        }
        break;

      case MOB_AI_WEED:
        valid = validmap[pos];
        for (dir = 0; dir < 4; ++dir) {
          if ((valid & dirvalid[dir]) && (mob = mobmap[POS_DIR(pos, dir)])) {
            goto doweed;
          }
        }
        return 0;

      doweed:
        mobbump(index, dir);
        hitmob(mob - 1, 1);
        return 1;

      case MOB_AI_REAPER:
        if (ai_dobump(index)) {
          return 1;
        } else {
          mob_target_pos[index] = mob_pos[PLAYER_MOB];
          dir = ai_getnextstep(index);
          if (dir != 255) {
            mobwalk(index, dir);
            return 1;
          }
        }
        break;

      case MOB_AI_ATTACK:
        mob_vis[index] = 1;
        if (ai_dobump(index)) {
          return 1;
        } else if (ai_tcheck(index)) {
          dir = ai_getnextstep(index);
          if (dir != 255) {
            mobwalk(index, dir);
            if (mob_type[index] == MOB_TYPE_GHOST && distmap[pos] > 1 &&
                XRND_33_PERCENT()) {
              mob_vis[index] = 0;
            }
            return 1;
          }
        }
        break;

      case MOB_AI_QUEEN:
        if (!mobsightmap[pos]) {
          mob_task[index] = MOB_AI_WAIT;
          mob_active[index] = 0;
          addfloat(pos, FLOAT_LOST);
        } else {
          mob_target_pos[index] = mob_pos[PLAYER_MOB];
          if (mob_charge[index] == 0) {
            dir = ai_getnextstep(index);
            if (dir != 255) {
              mob = num_mobs;
              addmob(MOB_TYPE_SLIME, pos);
              mobhopnew(mob, POS_DIR(pos, dir));
              mobmap[pos] = index + 1; // Fix mobmap back to queen
              mob_charge[index] = QUEEN_CHARGE_MOVES;
              return 1;
            }
          } else {
            --mob_charge[index];
            dir = ai_getnextstep_rev(index);
            if (dir != 255) {
              mobwalk(index, dir);
              return 1;
            }
          }
        }
        break;

      case MOB_AI_KONG:
        if (ai_dobump(index)) {
          return 1;
        } else if (ai_tcheck(index)) {
          // Get the movement direction and (partially) populate the distance
          // map
          dir = ai_getnextstep(index);

          // check for a pounce
          u8 tpos = mob_target_pos[index];
          u8 x = pos & 0xf, y = pos & 0xf0;
          u8 tx = tpos & 0xf, ty = tpos & 0xf0;
          // If x or y coordinate is the same...
          if (x == tx || y == ty) {
            // Find the direction from mob to target
            u8 sdir;
            if (tx < x) {
              sdir = DIR_LEFT;
            } else if (tx > x) {
              sdir = DIR_RIGHT;
            } else if (ty < y) {
              sdir = DIR_UP;
            } else { // ty > y
              sdir = DIR_DOWN;
            }

            // Check whether we can jump all the way to the target
            u8 spos = pos;
            while (spos != tpos && !IS_WALL_POS(spos)) {
              spos = POS_DIR(spos, sdir);
              if (distmap[spos] == 2 && !IS_SMARTMOB_POS(spos)) {
                // found it, jump here
                mobdir(index, sdir);
                mobhop(index, spos);
                mob_trigger[index] = 1;
                // override the anim state so we can do a bump when the hop
                // animation finishes
                mob_anim_state[index] = MOB_ANIM_STATE_POUNCE;
                sfx(SFX_TELEPORT);
                return 1;
              }
            }
          }

          // no pounce, try to move normally
          if (dir != 255) {
            mobwalk(index, dir);
            return 1;
          }
        }
        break;
    }
  }
  return 0;
}

u8 ai_dobump(u8 index) {
  u8 pos, diff, dir;
  pos = mob_pos[index];
  diff = mob_pos[PLAYER_MOB] - pos;

  // Use mobsightmap as a quick check to determine whether this mob is close to
  // the player.
  if (mobsightmap[pos]) {
    for (dir = 0; dir < 4; ++dir) {
      if (diff == dirpos[dir]) {
        goto ok;
      }
    }
    return 0;

ok:
    mobbump(index, dir);
    hitmob(PLAYER_MOB, 1);
    if (mob_type[index] == MOB_TYPE_SCORPION) {
      blind();
    }
    return 1;
  }
  return 0;
}

u8 ai_tcheck(u8 index) {
  ++mob_ai_cool[index];
  if (mobsightmap[mob_pos[index]]) {
    mob_target_pos[index] = mob_pos[PLAYER_MOB];
    mob_ai_cool[index] = 0;
  }
  if (mob_target_pos[index] == mob_pos[index] ||
      mob_ai_cool[index] > AI_COOL_MOVES) {
    addfloat(mob_pos[index], FLOAT_LOST);
    mob_task[index] = MOB_AI_WAIT;
    mob_active[index] = 0;
    return 0;
  }
  return 1;
}

u8 ai_getnextstep(u8 index) {
  u8 pos, newpos, bestval, bestdir, dist, valid, i;
  pos = mob_pos[index];
  calcdist_ai(pos, mob_target_pos[index]);
  bestval = bestdir = 255;
  valid = validmap[pos];
  const u8 *dir_perm = permutation_4 + (randint(24) << 2);

  for (i = 0; i < 4; ++i) {
    u8 dir = *dir_perm++;
    newpos = POS_DIR(pos, dir);
    if ((valid & dirvalid[dir]) && !IS_SMARTMOB_POS(newpos) &&
        (dist = distmap[newpos]) && dist < bestval) {
      bestval = dist;
      bestdir = dir;
    }
  }
  return bestdir;
}

// TODO: combine with above?
u8 ai_getnextstep_rev(u8 index) {
  u8 pos, newpos, bestval, bestdir, dist, valid, i;
  pos = mob_pos[index];
  calcdist_ai(pos, mob_target_pos[index]);
  bestval = 0;
  bestdir = 255;
  valid = validmap[pos];
  const u8 *dir_perm = permutation_4 + (randint(24) << 2);

  for (i = 0; i < 4; ++i) {
    u8 dir = *dir_perm++;
    newpos = POS_DIR(pos, dir);
    if ((valid & dirvalid[dir]) && !IS_SMARTMOB_POS(newpos) &&
        (dist = distmap[newpos]) && dist > bestval) {
      bestval = dist;
      bestdir = dir;
    }
  }
  return bestdir;
}

void sight(void) NONBANKED {
  u8 ppos, pos, adjpos, diff, head, oldtail, newtail, sig, valid, first,
      dist, maxdist, dir;
  memset(mobsightmap, 0, sizeof(mobsightmap));

  ppos = mob_pos[PLAYER_MOB];

  cands[head = 0] = 0;
  newtail = 1;
  first = 1;
  dist = 0;

  if (recover) {
    valid = validmap[ppos];
    if (valid & VALID_U) { cands[newtail++] = POS_U(0); }
    if (valid & VALID_L) { cands[newtail++] = POS_L(0); }
    if (valid & VALID_R) { cands[newtail++] = POS_R(0); }
    if (valid & VALID_D) { cands[newtail++] = POS_D(0); }
    maxdist = 1;
  } else {
    maxdist = 255;
  }

  do {
    oldtail = newtail;
    do {
      diff = cands[head++];
      mobsightmap[pos = (u8)(ppos + diff)] = 1;

      // Unfog this tile, within player sight range
      if (dist < maxdist && fogmap[pos]) {
        unfog_tile(pos);

        // Potentially start animating this tile
        if (IS_ANIMATED_POS(pos)) {
          *anim_tile_ptr++ = pos;

          // Store the offset into anim_tiles for this animated saw
          if ((tmap[pos] & TILE_SAW_MASK) == TILE_SAW) {
            sawmap[pos] = anim_tile_ptr - anim_tiles - 1;
          }
        }
      }

      if (first || !IS_OPAQUE_POS(pos)) {
        first = 0;
        valid = validmap[pos];

        // Unfog neighboring walls, within player sight range
        if (dist < maxdist) {
          for (dir = 0; dir < 4; ++dir) {
            if ((valid & dirvalid[dir]) && fogmap[adjpos = POS_DIR(pos, dir)] &&
                IS_WALL_POS(adjpos)) {
              unfog_tile(adjpos);
            }
          }
        }

        sig = sightsig[(u8)(68 + diff)] & valid;
        for (dir = 0; dir < 8; ++dir) {
          if (sig & dirvalid[dir]) { cands[newtail++] = POS_DIR(diff, dir); }
        }
      }
    } while(head != oldtail);
    ++dist;
  } while (oldtail != newtail);
}

void blind(void) {
  if (!recover) {
    recover = RECOVER_MOVES;

    // Reset fogmap, dtmap and sawmap
    memset(fogmap, 1, sizeof(fogmap));
    memset(dtmap, 0, sizeof(fogmap));
    memset(sawmap, 0, sizeof(sawmap));

    // Reset all tile animations (e.g. saws, torches)
    anim_tile_ptr = anim_tiles;

    // Use sight() instead of dosight=1, since the tile updates will be removed
    sight();
    doupdatemap = 1;
    // Clear all the mob updates later (since we may be updating all the mobs
    // right now)
    doblind = 1;
    sfx(SFX_BLIND);
  }
}

void animate(void) {
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

  if (animdone) {
    dopassturn = 1;
  }
}

void dirty_tile(u8 pos) NONBANKED {
  if (!dirtymap[pos]) {
    dirtymap[pos] = 1;
    *dirty_ptr++ = pos;
  }
}

void update_tile(u8 pos, u8 tile) {
  tmap[pos] = tile;
  flagmap[pos] = flags_bin[tile];
  dirty_tile(pos);
}

void unfog_tile(u8 pos) NONBANKED {
  dtmap[pos] = tmap[pos]; // TODO: only needed when blinded
  fogmap[pos] = 0;
  dirty_tile(pos);
}

void trigger_step(u8 mob) {
  u8 pos, tile, ptype, pindex, i, flt_start, flt_end, x, y, len, teleported;
  u16 equip_addr;

  teleported = 0;
redo:

  pos = mob_pos[mob];
  tile = tmap[pos];

  if (mob == PLAYER_MOB) {
    dosight = 1;

    if (tile == TILE_END) {
      dofadeout = doloadfloor = donextfloor = 1;
      sfx(SFX_STAIRS);
    }

    // Handle pickups
    if ((pindex = pickmap[pos])) {
      u8 sound = SFX_PICKUP;
      ptype = pick_type[--pindex];
      if (ptype == PICKUP_TYPE_HEART) {
        if (mob_hp[PLAYER_MOB] < 9) {
          ++mob_hp[PLAYER_MOB];
          set_vram_byte((u8 *)INV_HP_ADDR, TILE_0 + mob_hp[PLAYER_MOB]);
        }
        sound = SFX_HEART;
      } else if (ptype == PICKUP_TYPE_KEY) {
        if (num_keys < 9) {
          ++num_keys;
          set_vram_byte((u8 *)INV_KEYS_ADDR, TILE_0 + num_keys);
        }
      } else {
        len = pick_type_name_len[ptype];

        // First check whether this equipment already exists, so we can
        // increase the number of charges
        equip_addr = INV_EQUIP_ADDR;
        for (i = 0; i < MAX_EQUIPS; ++i) {
          if (equip_type[i] == ptype) {
            // Increase charges
            if (equip_charge[i] < 9) {
              equip_charge[i] += PICKUP_CHARGES;
              if (equip_charge[i] > 9) { equip_charge[i] = 9; }
              set_vram_byte((u8 *)(equip_addr + len - 2),
                            TILE_0 + equip_charge[i]);
            }
            goto pickup;
          }
          equip_addr += 32;
        }

        // Next check whether there is a new spot for this equipment
        equip_addr = INV_EQUIP_ADDR;
        for (i = 0; i < MAX_EQUIPS; ++i) {
          if (equip_type[i] == PICKUP_TYPE_NONE) {
            // Use this slot
            equip_type[i] = ptype;
            equip_charge[i] = PICKUP_CHARGES;
            vram_copy(equip_addr,
                      pick_type_name_tile + pick_type_name_start[ptype], len);

            // Update the inventory message if the selection was set to this
            // equip slot
            if (inv_select == i) {
              inv_selected_pick = equip_type[inv_select];
              inv_msg_update = 1;
            }
            goto pickup;
          }
          equip_addr += 32;
        }

        // No room, display "full!"
        ptype = PICKUP_TYPE_FULL;
        sound = SFX_OOPS;
        goto done;
      }

    pickup:
      delpick(pindex);

    done:
      sfx(sound);
      flt_start = float_pick_type_start[ptype];
      flt_end = float_pick_type_start[ptype + 1];
      x = POS_TO_X(pos) + float_pick_type_x_offset[ptype];
      y = POS_TO_Y(pos);

      for (i = flt_start; i < flt_end; ++i) {
        *next_float++ = y;
        *next_float++ = x;
        *next_float++ = float_pick_type_tiles[i];
        *next_float++ = FLOAT_FRAMES; // hijack prop for float timer
        x += 8;
      }
    }

    // Expose the exit button for this void region.
    u8 room;
    if ((room = roommap[pos]) > num_rooms) {
      room -= num_rooms + 1;
      u8 exitpos = void_exit[room];
      u8 exittile = tmap[exitpos];
      if (TILE_HAS_CRACKED_VARIANT(exittile)) {
        exittile += TILE_VOID_EXIT_DIFF;
        update_tile(exitpos, exittile);
      }
    }
  }

  // Step on saw
  if ((tile & TILE_SAW_MASK) == TILE_SAW) {
    u8 hp = mob_hp[mob];
    hitmob(mob, 3);
    if (mob != PLAYER_MOB && hp <= 3) {
      droppick_rnd(pos);
    }
    update_tile(pos, TILE_SAW_BROKEN);
    nop_saw_anim(pos);
  } else if (tile == TILE_TELEPORTER && !teleported) {
    u8 newpos = 0;
    do {
      if (tmap[newpos] == TILE_TELEPORTER && !IS_MOB(newpos)) {
        break;
      }
    } while(++newpos);

    if (pos != newpos) {
      // Move mob position in mobmap
      mobmap[pos] = 0;
      mobmap[newpos] = mob + 1;
      mob_pos[mob] = newpos;
      dirty_tile(pos);

      teleported = 1;
      sfx(SFX_TELEPORT);
      goto redo;
    }
  }
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

void inv_animate(void) {
  if (inv_anim_timer) {
    --inv_anim_timer;
    WY_REG = inv_anim_up ? inventory_up_y[inv_anim_timer]
                         : inventory_down_y[inv_anim_timer];
  }

  if (inv_anim_timer || inv_anim_up) {
    if (--inv_select_timer == 0) {
      inv_select_timer = INV_SELECT_FRAMES;
      ++inv_select_frame;
    }
    *next_sprite++ = WY_REG + INV_SELECT_Y_OFFSET + (inv_select << 3);
    *next_sprite++ = INV_SELECT_X_OFFSET + pickbounce[inv_select_frame & 7];
    *next_sprite++ = 0;
    *next_sprite++ = 0;

    if (inv_msg_update) {
      inv_msg_update = 0;

      u16 addr = INV_DESC_ADDR;
      u8 pick = equip_type[inv_select];
      u8 i;
      for (i = 0; i < NUM_INV_ROWS; ++i) {
        u8 len = pick_type_desc_len[pick][i];
        if (len) {
          vram_copy(addr, pick_type_desc_tile + pick_type_desc_start[pick][i],
                    len);
        }

        // Fill the rest of the row with 0
        vmemset((u8 *)addr + len, 0, INV_ROW_LEN - len);
        addr += 32;
      }
    }
  }
}

void update_obj1_pal(void) {
  if (--obj_pal1_timer == 0) {
    obj_pal1_timer = OBJ1_PAL_FRAMES;
    OBP1_REG = obj_pal1[obj_pal1_index = (obj_pal1_index + 1) & 7];
  }
}

void fadeout(void) NONBANKED {
  u8 i, j;
  i = 3;
  do {
    for (j = 0; j < FADE_FRAMES; ++j) { wait_vbl_done(); }
    BGP_REG = OBP0_REG = OBP1_REG = fadepal[i];
  } while(i--);
}

void fadein(void) NONBANKED {
  u8 i, j;
  i = 0;
  do {
    for (j = 0; j < FADE_FRAMES; ++j) { wait_vbl_done(); }
    BGP_REG = OBP0_REG = OBP1_REG = fadepal[i];
  } while(++i != 4);
}

u8 dropspot(u8 pos) {
  u8 i = 0, newpos;
  do {
    if ((newpos = is_valid(pos, drop_diff[i])) &&
        !(IS_SMARTMOB_POS(newpos) || pickmap[newpos])) {
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


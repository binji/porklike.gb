#include <gb/gb.h>
#include <gb/gbdecompress.h>
#include <string.h>

#pragma bank 1

#include "main.h"
#include "out/tilebg.h"
#include "out/tileshared.h"
#include "out/tilesprites.h"
#include "out/tiledead.h"

#define MAX_CANDS 256
#define MAX_DEAD_MOBS 4
#define MAX_MOBS 32    /* XXX Figure out what this should be */
#define MAX_PICKUPS 16 /* XXX Figure out what this should be */
#define MAX_FLOATS 8   /* For now; we'll probably want more */
#define MAX_EQUIPS 4
#define MAX_SPRS 16 /* Animation sprites for explosions, etc. */
#define MAX_SAWS 32 /* XXX */

#define BASE_FLOAT 18

#define MSG_REAPER 0
#define MSG_REAPER_Y 83
#define MSG_KEY 18
#define MSG_KEY_Y 80
#define MSG_LEN 18

// TODO: how big to make these arrays?
#define TILE_CODE_SIZE 128
#define MOB_TILE_CODE_SIZE 256

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
#define GAMEOVER_X_OFFSET 5
#define GAMEOVER_Y_OFFSET 2
#define GAMEOVER_WIDTH 9
#define GAMEOVER_HEIGHT 13
#define GAMEOVER_FLOOR_ADDR 0x992c
#define GAMEOVER_STEPS_ADDR 0x994c
#define GAMEOVER_KILLS_ADDR 0x996c

#define TILE_ANIM_FRAMES 8
#define TILE_ANIM_FRAME_DIFF 16
#define TILE_FLIP_DIFF 0x21
#define TILE_MOB_FLASH_DIFF 0x46
#define TILE_VOID_EXIT_DIFF 0x35
#define TILE_VOID_OPEN_DIFF 3

#define FADE_FRAMES 8
#define INV_ANIM_FRAMES 25

#define WALK_FRAMES 8
#define BUMP_FRAMES 4
#define MOB_HOP_FRAMES 8
#define PICK_HOP_FRAMES 8
#define FLOAT_FRAMES 70
#define OBJ1_PAL_FRAMES 8
#define MOB_FLASH_FRAMES 20
#define DEAD_MOB_FRAMES 10
#define MSG_FRAMES 120
#define INV_SELECT_FRAMES 8
#define INV_TARGET_FRAMES 2

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

#define PICKUP_FRAMES 6
#define PICKUP_CHARGES 2

#define IS_MOB(pos) (mobmap[pos])
#define IS_WALL_OR_MOB(tile, pos) (IS_WALL_TILE(tile) || mobmap[pos])
#define IS_MOB_AI(tile, pos)                                                   \
  ((flags_bin[tile] & 3) || (mobmap[pos] && !mob_active[mobmap[pos] - 1]))
#define IS_UNSPECIAL_WALL_TILE(tile)                                           \
  ((flags_bin[tile] & 0b00000011) == 0b00000001)

#define MAP_X_OFFSET 2
#define MAP_Y_OFFSET 0
#define POS_TO_ADDR(pos) (0x9802 + (((pos)&0xf0) << 1) + ((pos)&0xf))
#define POS_TO_X(pos) (((pos & 0xf) << 3) + 24)
#define POS_TO_Y(pos) (((pos & 0xf0) >> 1) + 16)

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

typedef enum MobAnimState {
  MOB_ANIM_STATE_NONE = 0,
  MOB_ANIM_STATE_WALK = 1,
  MOB_ANIM_STATE_BUMP1 = 2,
  MOB_ANIM_STATE_BUMP2 = 3,
  MOB_ANIM_STATE_HOP4 = 4,
  MOB_ANIM_STATE_HOP4_MASK = 7,
  // Use 8 here so we can use & to check for HOP4 or POUNCE
  MOB_ANIM_STATE_POUNCE = MOB_ANIM_STATE_HOP4 | 8,
} MobAnimState;

void gameinit(void);
void reset_anim_code(void);
void do_turn(void);
void pass_turn(void);
void begin_animate(void);
void move_player(void);
void use_pickup(void);
u8 shoot(u8 pos, u8 hit, u8 tile, u8 prop);
u8 shoot_dist(u8 pos, u8 hit);
u8 rope(u8 from, u8 to);
void mobdir(u8 index, u8 dir);
void mobwalk(u8 index, u8 dir);
void mobbump(u8 index, u8 dir);
void mobhop(u8 index, u8 pos);
void mobhopnew(u8 index, u8 pos);
void pickhop(u8 index, u8 pos);
void hitmob(u8 index, u8 dmg);
void hitpos(u8 pos, u8 dmg, u8 stun);
void update_wall_face(u8 pos);
void do_ai(void);
void do_ai_weeds(void);
u8 do_mob_ai(u8 index);
u8 ai_dobump(u8 index);
u8 ai_tcheck(u8 index);
u8 ai_getnextstep(u8 index);
u8 ai_getnextstep_rev(u8 index);
void blind(void);
void calcdist_ai(u8 from, u8 to);
void do_animate(void);
void end_animate(void);
void hide_sprites(void);
void set_tile_during_vbl(u8 pos, u8 tile);
void set_digit_tile_during_vbl(u16 addr, u8 value);
void set_tile_range_during_vbl(u16 addr, u8* source, u8 len);
void update_tile(u8 pos, u8 tile);
void update_tile_if_unfogged(u8 pos, u8 tile);
void unfog_tile(u8 pos);
void vbl_interrupt(void);

void trigger_step(u8 index);

void addfloat(u8 pos, u8 tile);
void update_floats_and_msg_and_sprs(void);
void showmsg(u8 index, u8 y);

void inv_animate(void);

void update_obj1_pal(void);
void fadeout(void);
void fadein(void);

void delmob(u8 index);
void delpick(u8 index);
u8 is_valid(u8 pos, u8 diff) __preserves_regs(b, c);
void droppick(PickupType type, u8 pos);
void droppick_rnd(u8 pos);
u8 dropspot(u8 pos);

void adddeadmob(u8 index);

u8 addspr(u8 speed, u16 x, u16 y, u16 dx, u16 dy, u8 drag, u8 timer, u8 prop);

void nop_saw_anim(u8 pos);

Map tmap;        // Tile map (everything unfogged)
Map dtmap;       // Display tile map (w/ fogged tiles)
Map roommap;     // Room/void map
Map distmap;     // Distance map
Map mobmap;      // Mob map
Map pickmap;     // Pickup map
Map sigmap;      // Signature map (tracks neighbors) **only used during mapgen
Map tempmap;     // Temp map (used for carving)      **only used during mapgen
Map mobsightmap; // Always checks 4 steps from player
Map fogmap;      // Tiles player hasn't seen
Map sawmap;      // Map from saw to its animation code addr in tile_code

u8 room_pos[MAX_ROOMS];
u8 room_w[MAX_ROOMS];
u8 room_h[MAX_ROOMS];
u8 room_avoid[MAX_ROOMS]; // ** only used during mapgen
u8 num_rooms;

u8 start_room;
u8 floor;
u8 startpos;

u8 cands[MAX_CANDS];
u8 num_cands;

MobType mob_type[MAX_MOBS];
u8 mob_anim_frame[MAX_MOBS]; // Index into mob_type_anim_frames
u8 mob_anim_timer[MAX_MOBS]; // 0..mob_anim_speed[type], how long between frames
u8 mob_anim_speed[MAX_MOBS]; // current anim speed (changes when moving)
u8 mob_pos[MAX_MOBS];
u8 mob_x[MAX_MOBS], mob_y[MAX_MOBS];   // x,y location of sprite
u8 mob_dx[MAX_MOBS], mob_dy[MAX_MOBS]; // dx,dy moving sprite
u8 mob_clear_tile[MAX_MOBS];           // whether to remove the mob tile
u8 mob_clear_pos[MAX_MOBS];            // which position to clear
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

PickupType pick_type[MAX_PICKUPS];
u8 pick_pos[MAX_PICKUPS];
u8 pick_anim_frame[MAX_PICKUPS];             // Index into pick_type_anim_frames
u8 pick_anim_timer[MAX_PICKUPS];             // 0..pick_type_anim_speed[type]
u8 pick_x[MAX_PICKUPS], pick_y[MAX_PICKUPS]; // x,y location of sprite
u8 pick_dx[MAX_PICKUPS], pick_dy[MAX_PICKUPS]; // dx,dy moving sprite
u8 pick_move_timer[MAX_PICKUPS];               // timer for sprite animation
u8 num_picks;

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

u8 tile_code[TILE_CODE_SIZE];         // code for animating tiles
u8 mob_tile_code[MOB_TILE_CODE_SIZE]; // code for animating mobs
u8 *tile_code_ptr;                    // next available index in tile_code
u16 last_tile_addr;                   // last tile address in HL for tile_code
u8 last_tile_val;                     // last tile value in A for tile_code
u8 tile_timer; // timer for animating tiles (every TILE_ANIM_FRAMES frames)
u8 tile_inc;   // either 0 or TILE_ANIM_FRAME_DIFF, for two frames of animation
u8 *mob_tile_code_ptr;  // next available index in mob_tile_code
u16 mob_last_tile_addr; // last tile address in HL for mob_tile_code
u8 mob_last_tile_val;   // last tile value in A for mob_tile_code

u8 float_time[MAX_FLOATS];
u8 next_float;

u8 msg_timer;

Turn turn;
u8 noturn;
u8 gameover;
u8 gameover_timer;

u8 is_targeting;
Dir target_dir;

u8 joy, lastjoy, newjoy;
u8 doupdatemap, doupdatewin, dofadeout, doloadfloor, donextfloor, doblind,
    dosight;
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
  LCDC_REG = 0b11100011;  // display on, window/bg/obj on, window@9c00
  gb_decompress_bkg_data(0x80, shared_bin);
  gb_decompress_sprite_data(0, sprites_bin);
  add_VBL(vbl_interrupt);
  gameinit();
  SWITCH_ROM_MBC1(3);
  mapgen();
  SWITCH_ROM_MBC1(1);
  doupdatemap = 1;
  fadein();

  while(1) {
    lastjoy = joy;
    joy = joypad();
    newjoy = (joy ^ lastjoy) & joy;

    if (gameover) {
      if (gameover == 1) {
        gameover = 2;
        hide_sprites();
        fadeout();
        IE_REG &= ~VBL_IFLAG;

        // Hide window
        move_win(23, 160);
        reset_anim_code();
        gb_decompress_bkg_data(0, dead_bin);
        init_bkg(0);
        set_bkg_tiles(GAMEOVER_X_OFFSET, GAMEOVER_Y_OFFSET, GAMEOVER_WIDTH,
                      GAMEOVER_HEIGHT, gameover_map);
        counter_out(&st_floor, GAMEOVER_FLOOR_ADDR);
        counter_out(&st_steps, GAMEOVER_STEPS_ADDR);
        counter_out(&st_kills, GAMEOVER_KILLS_ADDR);

        IE_REG |= VBL_IFLAG;
        fadein();
      } else {
        // Wait for keypress
        if (newjoy & J_A) {
          gameover = 0;
          doloadfloor = 1;
          fadeout();

          // reset initial state
          gameinit();
        }
      }
    } else {
      if (newjoy & J_START) { // XXX cheat
        dofadeout = donextfloor = doloadfloor = 1;
      }

      if (dofadeout) {
        dofadeout = 0;
        fadeout();
      }
      if (donextfloor) {
        donextfloor = 0;
        ++floor;
        counter_inc(&st_floor);

        // Set back to FLOOR #
        if (recover != 0) {
          vram_copy(INV_FLOOR_ADDR, inventory_map + INV_FLOOR_OFFSET,
                    INV_FLOOR_3_SPACES_LEN);
          recover = 0;
        }
      }
      if (doloadfloor) {
        doloadfloor = 0;
        hide_sprites();
        IE_REG &= ~VBL_IFLAG;
        SWITCH_ROM_MBC1(3);
        mapgen();
        SWITCH_ROM_MBC1(1);
        reset_anim_code();
        IE_REG |= VBL_IFLAG;
        doupdatemap = 1;
        fadein();
      }

      begin_animate();

      if (gameover_timer) {
        if (--gameover_timer == 0) {
          gameover = 1;
        }
      } else {
        do_turn();
      }

      do_animate();
      update_floats_and_msg_and_sprs();
      inv_animate();
      end_animate();
      update_obj1_pal();
    }
    wait_vbl_done();
  }
}

void gameinit(void) {
  reset_anim_code();
  tile_timer = TILE_ANIM_FRAMES;

  // Reset scroll registers
  SCX_REG = SCY_REG = 0;
  // Reload bg tiles
  gb_decompress_bkg_data(0, bg_bin);
  init_bkg(0);

  // Reset player mob
  num_mobs = 0;
  addmob(MOB_TYPE_PLAYER, 0);
  set_vram_byte((void*)INV_HP_ADDR, TILE_0 + mob_hp[PLAYER_MOB]);

  turn = TURN_PLAYER;
  next_float = BASE_FLOAT;

  // Set up inventory window
  move_win(23, 128);
  inv_select_timer = INV_SELECT_FRAMES;
  inv_target_timer = INV_TARGET_FRAMES;
  inv_msg_update = 1;
  memset(equip_type, PICKUP_TYPE_NONE, sizeof(equip_type));
  doupdatewin = 1;

  floor = 0;
  counter_zero(&st_floor);
  counter_zero(&st_steps);
  counter_zero(&st_kills);
}

void reset_anim_code(void) {
  tile_code[0] = mob_tile_code[0] = 0xc9; // ret
}

void begin_animate(void) {
  mob_last_tile_addr = 0;
  mob_last_tile_val = 0xff;
  mob_tile_code_ptr = mob_tile_code;
  // Initialize to ret so if a VBL occurs while we're setting the animation it
  // won't run off the end
  *mob_tile_code_ptr++ = 0xc9; // ret

  // Start mob sprites after floats
  next_sprite = (u8*)shadow_OAM + ((BASE_FLOAT + MAX_FLOATS) << 2);
}

void end_animate(void) {
  // Hide the rest of the sprites
  while (next_sprite < last_next_sprite) {
    *next_sprite++ = 0;  // hide sprite
    next_sprite += 3;
  }
  last_next_sprite = next_sprite;

  // When blinded, don't update any mob animations
  if (!doblind) {
    *mob_tile_code_ptr++ = 0xf1; // pop af
    *mob_tile_code_ptr++ = 0xc9; // ret
    mob_tile_code[0] = 0xf5;     // push af
  }
}

void hide_sprites(void) NONBANKED {
  memset(shadow_OAM, 0, 160);
  next_float = BASE_FLOAT;
}

void do_turn(void) {
  if (inv_anim_up) {
    if (!inv_anim_timer) {
      if (newjoy & (J_UP | J_DOWN)) {
        if (newjoy & J_UP) {
          inv_select += 3;
        } else if (newjoy & J_DOWN) {
          ++inv_select;
        }
        inv_select &= 3;
        inv_msg_update = 1;
        inv_selected_pick = equip_type[inv_select];
      }
      if (newjoy & (J_B | J_A)) {
        inv_anim_timer = INV_ANIM_FRAMES;
        inv_anim_up ^= 1;
        is_targeting =
            ((newjoy & J_A) != 0) && inv_selected_pick != PICKUP_TYPE_NONE;
      }
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
      do_ai(); // XXX maybe recursive
    }
    break;

  case TURN_AI:
    turn = TURN_WEEDS;
    do_ai(); // XXX maybe recursive
    break;

  case TURN_WEEDS:
    turn = TURN_PLAYER;
    if (floor && steps == REAPER_AWAKENS_STEPS) {
      ++steps; // Make sure to only spawn reaper once per floor
      addmob(MOB_TYPE_REAPER, dropspot(startpos));
      mob_active[num_mobs - 1] = 1;
      showmsg(MSG_REAPER, MSG_REAPER_Y);
    }
    if (doblind) {
      // Update floor to display recover count instead
      set_tile_range_during_vbl(INV_FLOOR_ADDR, blind_map, INV_BLIND_LEN);
      counter_thirty(&st_recover);
      counter_out(&st_recover, INV_FLOOR_NUM_ADDR);
      doblind = 0;
    } else if (recover) {
      if (--recover == 0) {
        // Set back to FLOOR #
        set_tile_range_during_vbl(INV_FLOOR_ADDR,
                                  inventory_map + INV_FLOOR_OFFSET,
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
    if (joy & (J_LEFT | J_RIGHT | J_UP | J_DOWN)) {
      if (joy & J_LEFT) {
        dir = DIR_LEFT;
      } else if (joy & J_RIGHT) {
        dir = DIR_RIGHT;
      } else if (joy & J_UP) {
        dir = DIR_UP;
      } else {
        dir = DIR_DOWN;
      }

      if (is_targeting) {
        if (dir != target_dir) {
          target_dir = dir;
          mobdir(PLAYER_MOB, dir);
          mob_anim_timer[PLAYER_MOB] = 1;  // Update the player's direction
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
                    set_digit_tile_during_vbl(INV_KEYS_ADDR, num_keys);
                    dosight = 1;
                    goto dobump;
                  } else {
                    showmsg(MSG_KEY, MSG_KEY_Y);
                  }
                } else if (tile == TILE_VOID_BUTTON_U ||
                           tile == TILE_VOID_BUTTON_L ||
                           tile == TILE_VOID_BUTTON_R ||
                           tile == TILE_VOID_BUTTON_D) {
                  update_tile(newpos, tile + TILE_VOID_OPEN_DIFF);
                  update_wall_face(newpos);
                  dosight = 1;
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
    } else if (is_targeting && (newjoy & J_A)) {
      use_pickup();
      turn = TURN_PLAYER_MOVED;
    } else if (is_targeting && (newjoy & J_B) || (newjoy & J_A)) {
      // Open inventory (or back out of targeting)
      inv_anim_timer = INV_ANIM_FRAMES;
      inv_anim_up ^= 1;
      is_targeting = 0;
    }
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
    if (valid) {
      land = hit;
      hit = POS_DIR(hit, target_dir);
    }
  } while(valid && !IS_WALL_OR_MOB(tmap[hit], hit));

  u8 mob1 = mobmap[pos1];
  u8 mob2 = mobmap[pos2];
  u8 mobh = mobmap[hit];

  u8 spr;
  switch (inv_selected_pick) {
    case PICKUP_TYPE_JUMP:
      if (valid2 && !IS_WALL_OR_MOB(tmap[pos2], pos2)) {
        if (mob1) {
          mob_stun[mob1 - 1] = 1;
        }
        mobhop(PLAYER_MOB, pos2);
      }
      break;

    case PICKUP_TYPE_BOLT:
      spr = shoot(pos, hit, TILE_SHOT, 0);
      spr_type[spr] = SPR_TYPE_BOLT;
      spr_trigger_val[spr] = hit;
      break;

    case PICKUP_TYPE_PUSH:
      spr = shoot(pos, hit, TILE_SHOT, 0);
      spr_type[spr] = SPR_TYPE_PUSH;
      spr_trigger_val[spr] = mobh;
      break;

    case PICKUP_TYPE_GRAPPLE:
      spr = rope(pos, land);
      spr_type[spr] = SPR_TYPE_GRAPPLE;
      spr_trigger_val[spr] = land;
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
      break;

    case PICKUP_TYPE_SMASH:
      mobbump(PLAYER_MOB, target_dir);
      if (valid1) {
        hitpos(pos1, 2, 0);
        if (IS_BREAKABLE_WALL(tmap[pos1])) {
          update_tile(pos1, dirt_tiles[xrnd() & 3]);
          update_wall_face(pos1);
        }
      }
      break;

    case PICKUP_TYPE_HOOK:
      spr = rope(pos, hit);
      spr_type[spr] = SPR_TYPE_HOOK;
      spr_trigger_val[spr] = mobh;
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
      break;
    }

    case PICKUP_TYPE_SUPLEX:
      if (valid1 && validb && mob1 && !IS_WALL_OR_MOB(tmap[posb], posb)) {
        mobhop(mob1 - 1, posb);
        mob_stun[mob1 - 1] = 1;
        mob_trigger[mob1 - 1] = 1;
        mobbump(PLAYER_MOB, target_dir);
      }
      break;

    case PICKUP_TYPE_SLAP:
      if (validb) {
        if (IS_WALL_OR_MOB(tmap[posb], posb)) {
          mobbump(PLAYER_MOB, invdir[target_dir]);
        } else {
          mobhop(PLAYER_MOB, posb);
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
    inv_msg_update = 1;

    // Clear equip display
    vram_copy(equip_addr, inventory_map + INV_BLANK_ROW_OFFSET, INV_ROW_LEN);
  } else {
    // Update charge count display
    set_vram_byte((void *)(equip_addr + pick_type_name_len[inv_selected_pick]),
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
  mob_clear_tile[index] = 1;
  mob_clear_pos[index] = pos;

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
  mob_clear_tile[index] = 1;
  mob_clear_pos[index] = pos;

  mob_move_timer[index] = BUMP_FRAMES;
  mob_anim_state[index] = MOB_ANIM_STATE_BUMP1;
  mobdir(index, dir);
}

void mobhop(u8 index, u8 newpos) {
  u8 pos = mob_pos[index];
  mobhopnew(index, newpos);
  mob_clear_tile[index] = 1;
  mob_clear_pos[index] = pos;
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

void hitmob(u8 index, u8 dmg) {
  u8 mob, mtype, percent, tile, slime, pos, valid, i;
  u16 x, y;
  pos = mob_pos[index];
  mtype = mob_type[index];

  if (mob_type_object[mtype]) {
    if (mtype == MOB_TYPE_CHEST || mtype == MOB_TYPE_HEART_CHEST) {
      percent = 100;
      tile = TILE_CHEST_OPEN;
      slime = 0;
    } else if (mtype == MOB_TYPE_BOMB) {
      percent = 100;
      tile = TILE_BOMB_EXPLODED;
      slime = 0;
    } else {
      percent = 20;
      tile = dirt_tiles[xrnd() & 3];
      slime = 1;
    }

    delmob(index);
    if (!IS_SPECIAL_TILE(tmap[pos])) {
      update_tile(pos, tile);
    }

    if (randint(100) < percent) {
      if (slime && XRND_20_PERCENT()) {
        mob = num_mobs;
        addmob(MOB_TYPE_SLIME, pos);
        mobhopnew(mob, dropspot(pos));
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
  } else {
    addfloat(pos, float_dmg[dmg]);

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
      set_tile_during_vbl(pos, dtmap[pos]);

      if (index == PLAYER_MOB) {
        set_digit_tile_during_vbl(INV_HP_ADDR, 0);
        gameover_timer = GAMEOVER_FRAMES;
      } else {
        // Killing a reaper restores the your health to 5 and drops a free key
        if (mob_type[index] == MOB_TYPE_REAPER) {
          if (mob_hp[PLAYER_MOB] < 5) {
            mob_hp[PLAYER_MOB] = 5;
            set_digit_tile_during_vbl(INV_HP_ADDR, mob_hp[PLAYER_MOB]);
          }
          droppick(PICKUP_TYPE_KEY, pos);
        }
        counter_inc(&st_kills);
      }
    } else {
      mob_flash[index] = MOB_FLASH_FRAMES;
      mob_hp[index] -= dmg;
      mob_vis[index] = 1;
      if (index == PLAYER_MOB) {
        set_digit_tile_during_vbl(INV_HP_ADDR, mob_hp[PLAYER_MOB]);
      }
    }
  }
}

void hitpos(u8 pos, u8 dmg, u8 stun) {
  u8 tile, mob;
  tile = tmap[pos];
  if ((mob = mobmap[pos])) {
    hitmob(mob - 1, dmg); // XXX maybe recursive...
    if (stun) {
      mob_stun[mob - 1] = 1;
    }
  }
  if (tmap[pos] == TILE_SAW) {
    tile = TILE_SAW_BROKEN;
    nop_saw_anim(pos);
  } else if (IS_CRACKED_WALL_TILE(tile)) {
    tile = dirt_tiles[xrnd() & 3];
    update_wall_face(pos);
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
    update_tile_if_unfogged(pos, tile);
  }
}

void do_ai(void) {
  u8 moving = 0;
  u8 i;
  for (i = 0; i < num_mobs; ++i) {
    if ((mob_type[i] == MOB_TYPE_WEED) == (turn == TURN_WEEDS)) {
      moving |= do_mob_ai(i);
    }
  }
  if (!moving) {
    pass_turn();
  }
}

u8 do_mob_ai(u8 index) {
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
            u8 tile;
            while (spos != tpos && !IS_WALL_TILE(tile = tmap[spos])) {
              spos = POS_DIR(spos, sdir);
              if (distmap[spos] == 2 && !IS_SMARTMOB(tile, spos)) {
                // found it, jump here
                mobdir(index, sdir);
                mobhop(index, spos);
                mob_trigger[index] = 1;
                // override the anim state so we can do a bump when the hop
                // animation finishes
                mob_anim_state[index] = MOB_ANIM_STATE_POUNCE;
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
    if ((valid & dirvalid[dir]) && !IS_SMARTMOB(tmap[newpos], newpos) &&
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
    if ((valid & dirvalid[dir]) && !IS_SMARTMOB(tmap[newpos], newpos) &&
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

  // Put a "ret" at the beginning of the tile animation code in case it is
  // executed while we are modifying it.
  tile_code[0] = 0xc9;

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
        fogmap[pos] = 0;

        // Potentially start animating this tile
        if (IS_ANIMATED_TILE(tmap[pos])) {
          add_tile_anim(pos, tmap[pos]);

          // Store the offset into tile_code for this animated saw
          if (tmap[pos] == TILE_SAW) {
            sawmap[pos] = tile_code_ptr - tile_code - 1;
          }
        }
      }

      if (first || !IS_OPAQUE_TILE(tmap[pos])) {
        first = 0;
        valid = validmap[pos];

        // Unfog neighboring walls, within player sight range
        if (dist < maxdist) {
          for (dir = 0; dir < 4; ++dir) {
            if ((valid & dirvalid[dir]) && fogmap[adjpos = POS_DIR(pos, dir)] &&
                IS_WALL_TILE(tmap[adjpos])) {
              unfog_tile(adjpos);
              fogmap[adjpos] = 0;
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

  // Put the epilog back on, and replace the "ret" with "push af"
  end_tile_anim();
  tile_code[0] = 0xf5;
}

void blind(void) {
  u8 i, x, y;
  if (!recover) {
    recover = RECOVER_MOVES;

    // Display float
    x = POS_TO_X(mob_pos[PLAYER_MOB]) + FLOAT_BLIND_X_OFFSET;
    y = POS_TO_Y(mob_pos[PLAYER_MOB]);
    for (i = 0; i < 3; ++i) {
      shadow_OAM[next_float].x = x;
      shadow_OAM[next_float].y = y;
      shadow_OAM[next_float].tile = TILE_BLIND + i;
      float_time[next_float - BASE_FLOAT] = FLOAT_FRAMES;
      ++next_float;
      x += 8;
    }

    // Reset fogmap, dtmap and sawmap
    memset(fogmap, 1, sizeof(fogmap));
    memset(dtmap, 0, sizeof(fogmap));
    memset(sawmap, 0, sizeof(sawmap));

    // Reset all tile animations (e.g. saws, torches)
    begin_tile_anim();
    end_tile_anim();

    // Use sight() instead of dosight=1, since the tile updates will be removed
    sight();
    doupdatemap = 1;
    // Clear all the mob updates later (since we may be updating all the mobs
    // right now)
    doblind = 1;
  }
}

void calcdist_ai(u8 from, u8 to) {
  u8 pos, head, oldtail, newtail, valid, dist, newpos, maxdist, dir;
  cands[head = 0] = pos = to;
  newtail = 1;

  memset(distmap, 0, sizeof(distmap));
  dist = 0;
  maxdist = 255;

  do {
    oldtail = newtail;
    ++dist;
    do {
      pos = cands[head++];
      if (pos == from) { maxdist = dist + 1; }
      if (!distmap[pos]) {
        distmap[pos] = dist;
        valid = validmap[pos];
        for (dir = 0; dir < 4; ++dir) {
          if ((valid & dirvalid[dir]) && !distmap[newpos = POS_DIR(pos, dir)]) {
            if (!IS_MOB_AI(tmap[newpos], newpos)) { cands[newtail++] = newpos; }
          }
        }
      }
    } while (head != oldtail);
  } while (oldtail != newtail && dist != maxdist);
}

void do_animate(void) {
  u8 i, dosprite, dotile, sprite, prop, frame, animdone;
  animdone = num_sprs == 0;

  // Loop through all pickups
  for (i = 0; i < num_picks; ++i) {
    // Don't draw the pickup if it is fogged or a mob is standing on top of it
    if (fogmap[pick_pos[i]] || mobmap[pick_pos[i]]) { continue; }

    dotile = 0;
    if (tile_timer == 1 && IS_ANIMATED_TILE(tmap[pick_pos[i]])) {
      dotile = 1;
    }

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
    if (sprite && pick_y[i] <= WY_REG + 9) {
      *next_sprite++ = pick_y[i];
      *next_sprite++ = pick_x[i];
      *next_sprite++ = sprite;
      *next_sprite++ = 0;
    }

    if (dotile || pick_move_timer[i]) {
      frame = pick_type_anim_frames[pick_anim_frame[i]];
      if (pick_move_timer[i]) {
        animdone = 0;
        if (pick_y[i] <= WY_REG + 9) {
          *next_sprite++ = pick_y[i];
          *next_sprite++ = pick_x[i];
          *next_sprite++ = frame;
          *next_sprite++ = 0;
        }

        pick_x[i] += pick_dx[i];
        pick_y[i] += pick_dy[i] + hopy12[--pick_move_timer[i]];

        if (pick_move_timer[i] == 0) {
          set_tile_during_vbl(pick_pos[i], frame);
        }
      } else {
        set_tile_during_vbl(pick_pos[i], frame);
      }
    }
  }

  // Loop through all mobs, update animations
  for (i = 0; i < num_mobs; ++i) {
    u8 pos = mob_pos[i];

    if (mob_clear_tile[i]) {
      mob_clear_tile[i] = 0;
      set_tile_during_vbl(mob_clear_pos[i], dtmap[mob_clear_pos[i]]);
    }

    // TODO: need to properly handle active fogged mobs; they should still do
    // their triggers but not draw anything. We also probably want to display
    // mobs that are partially fogged; if the start or end position is
    // unfogged.
    if (fogmap[pos] || !mob_vis[i]) { continue; }

    dosprite = dotile = 0;
    if (tile_timer == 1 && IS_ANIMATED_TILE(tmap[pos])) {
      dotile = 1;
    }

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
      frame = mob_type_anim_frames[mob_anim_frame[i]];

      if (dosprite && mob_y[i] <= WY_REG + 9) {
        prop = mob_flip[i] ? S_FLIPX : 0;
        *next_sprite++ = mob_y[i];
        *next_sprite++ = mob_x[i];
        if (mob_flash[i]) {
          *next_sprite++ = frame - TILE_MOB_FLASH_DIFF;
          *next_sprite++ = S_PALETTE | prop;
          --mob_flash[i];
        } else {
          *next_sprite++ = frame;
          *next_sprite++ = prop | (i + 1 == key_mob ? S_PALETTE : 0);
        }
      }

      if (mob_flip[i]) {
        frame += TILE_FLIP_DIFF;
      }

      if (mob_move_timer[i]) {
        animdone = 0;
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

            case MOB_ANIM_STATE_WALK:
              goto done;

            done:
            case MOB_ANIM_STATE_BUMP2:
            case MOB_ANIM_STATE_HOP4:
              mob_anim_state[i] = MOB_ANIM_STATE_NONE;
              mob_dx[i] = mob_dy[i] = 0;
              set_tile_during_vbl(pos, frame);
              break;

            case MOB_ANIM_STATE_POUNCE:
              // bump player, if possible
              if (!ai_dobump(i)) {
                // Otherwise finish animation
                mob_anim_state[i] = MOB_ANIM_STATE_NONE;
                mob_dx[i] = mob_dy[i] = 0;
                set_tile_during_vbl(pos, frame);
              }
              break;
          }
          // Reset animation speed
          mob_anim_speed[i] = mob_type_anim_speed[mob_type[i]];

          if (mob_trigger[i]) {
            mob_trigger[i] = 0;
            trigger_step(i);
          }
        }
      } else if (dotile) {
        set_tile_during_vbl(pos, frame);
      }
    }
  }

  // Draw all dead mobs as sprites
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
    } else if (dmob_y[i] <= WY_REG + 9) {
      *next_sprite++ = dmob_y[i];
      *next_sprite++ = dmob_x[i];
      *next_sprite++ = dmob_tile[i];
      *next_sprite++ = dmob_prop[i];
      ++i;
    }
  }

  if (animdone) {
    pass_turn();
  }
}

void set_tile_during_vbl(u8 pos, u8 tile) NONBANKED {
  u16 addr = POS_TO_ADDR(pos);
  u8 *ptr = mob_tile_code_ptr;
  if ((addr >> 8) != (mob_last_tile_addr >> 8)) {
    *ptr++ = 0x21; // ld hl, addr
    *ptr++ = (u8)addr;
    *ptr++ = addr >> 8;
  } else {
    *ptr++ = 0x2e; // ld l, lo(addr)
    *ptr++ = (u8)addr;
  }
  mob_last_tile_addr = addr;
  if (tile != mob_last_tile_val) {
    *ptr++ = 0x3e; // ld a, tile
    *ptr++ = tile;
    mob_last_tile_val = tile;
  }
  *ptr++ = 0x77; // ld (hl), a
  mob_tile_code_ptr = ptr;
}

void set_digit_tile_during_vbl(u16 addr, u8 value) {
  u8 *ptr = mob_tile_code_ptr;
  *ptr++ = 0x21; // ld hl, addr
  *ptr++ = addr & 0xff;
  *ptr++ = addr >> 8;
  *ptr++ = 0x3e; // ld a, tile
  *ptr++ = mob_last_tile_val = TILE_0 + value;
  *ptr++ = 0x77; // ld (hl), a
  mob_last_tile_addr = INV_HP_ADDR;
  mob_tile_code_ptr = ptr;
}

void set_tile_range_during_vbl(u16 addr, u8* src, u8 len) {
  u8 *dst = mob_tile_code_ptr;
  *dst++ = 0x21; // ld hl, addr
  *dst++ = addr & 0xff;
  *dst++ = addr >> 8;
  do {
    *dst++ = 0x3e; // ld a, tile
    *dst++ = *src++;
    *dst++ = 0x22; // ld (hl+), a
  } while(--len);
  mob_last_tile_addr = addr + len;
  mob_last_tile_val = *--src;
  mob_tile_code_ptr = dst;
}

void update_tile(u8 pos, u8 tile) {
  set_tile_during_vbl(pos, dtmap[pos] = tmap[pos] = tile);
}

void update_tile_if_unfogged(u8 pos, u8 tile) {
  // If the tile is already unfogged then updated it, otherwise just
  // change tmap.
  if (fogmap[pos]) {
    tmap[pos] = tile;
  } else {
    update_tile(pos, tile);
  }
}

void unfog_tile(u8 pos) NONBANKED {
  set_tile_during_vbl(pos, dtmap[pos] = tmap[pos]);
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
    }

    // Handle pickups
    if ((pindex = pickmap[pos])) {
      ptype = pick_type[--pindex];
      if (ptype == PICKUP_TYPE_HEART) {
        if (mob_hp[PLAYER_MOB] < 9) {
          ++mob_hp[PLAYER_MOB];
          set_digit_tile_during_vbl(INV_HP_ADDR, mob_hp[PLAYER_MOB]);
        }
      } else if (ptype == PICKUP_TYPE_KEY) {
        if (num_keys < 9) {
          ++num_keys;
          set_digit_tile_during_vbl(INV_KEYS_ADDR, num_keys);
        }
      } else {
        // Is there a free spot in the equip?
        equip_addr = INV_EQUIP_ADDR;
        len = pick_type_name_len[ptype];
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
          } else if (equip_type[i] == ptype) {
            // Increase charges
            if (equip_charge[i] < 9) {
              equip_charge[i] += PICKUP_CHARGES;
              if (equip_charge[i] > 9) { equip_charge[i] = 9; }
              set_vram_byte((void *)(equip_addr + len - 2),
                            TILE_0 + equip_charge[i]);
            }
            goto pickup;
          }
          equip_addr += 32;
        }
        // No room, display "full!"
        ptype = PICKUP_TYPE_FULL;
        goto done;
      }

    pickup:
      delpick(pindex);

    done:
      flt_start = float_pick_type_start[ptype];
      flt_end = float_pick_type_start[ptype + 1];
      x = POS_TO_X(pos) + float_pick_type_x_offset[ptype];
      y = POS_TO_Y(pos);

      for (i = flt_start; i < flt_end; ++i) {
        shadow_OAM[next_float].x = x;
        shadow_OAM[next_float].y = y;
        shadow_OAM[next_float].tile = float_pick_type_tiles[i];
        float_time[next_float - BASE_FLOAT] = FLOAT_FRAMES;
        ++next_float;
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
        update_tile_if_unfogged(exitpos, exittile);
      }
    }
  }

  // Step on saw
  if (tile == TILE_SAW) {
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

      // Reset mob anim timer so the tile is updated next frame
      mob_anim_timer[mob] = 1;
      set_tile_during_vbl(pos, dtmap[pos]);

      teleported = 1;
      goto redo;
    }
  }
}

void vbl_interrupt(void) NONBANKED {
  if (doupdatemap) {
    // update floor number
    counter_out(&st_floor, INV_FLOOR_NUM_ADDR);
    set_bkg_tiles(MAP_X_OFFSET, MAP_Y_OFFSET, MAP_WIDTH, MAP_HEIGHT, dtmap);
    doupdatemap = 0;
  } else if (doupdatewin) {
    set_win_tiles(0, 0, INV_WIDTH, INV_HEIGHT, inventory_map);
    doupdatewin = 0;
  }
  if (--tile_timer == 0) {
    tile_timer = TILE_ANIM_FRAMES;
    tile_inc ^= TILE_ANIM_FRAME_DIFF;
    ((vfp)tile_code)();
  }

  ((vfp)mob_tile_code)();
}

void addfloat(u8 pos, u8 tile) {
  shadow_OAM[next_float].x = POS_TO_X(pos);
  shadow_OAM[next_float].y = POS_TO_Y(pos);
  shadow_OAM[next_float].tile = tile;
  float_time[next_float - BASE_FLOAT] = FLOAT_FRAMES;
  ++next_float;
}

void showmsg(u8 index, u8 y) {
  u8 i, j;
  u8* p = (u8*)shadow_OAM;
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

void update_floats_and_msg_and_sprs(void) {
  u8 i, j;
  // Hide message if timer runs out
  if (msg_timer && --msg_timer == 0) {
    for (i = 0; i < MSG_LEN; ++i) {
      hide_sprite(i);
    }
  }

  // Draw float sprites and remove them if timed out
  for (i = BASE_FLOAT; i < next_float;) {
    j = i - BASE_FLOAT;
    --float_time[j];
    if (float_time[j] == 0) {
      --next_float;
      if (next_float != BASE_FLOAT) {
        shadow_OAM[i].x = shadow_OAM[next_float].x;
        shadow_OAM[i].y = shadow_OAM[next_float].y;
        shadow_OAM[i].tile = shadow_OAM[next_float].tile;
        float_time[j] = float_time[next_float - BASE_FLOAT];
        hide_sprite(next_float);
      } else {
        hide_sprite(i);
      }
      continue;
    } else {
      shadow_OAM[i].y -= float_diff_y[float_time[j]];
    }
    ++i;
  }

  // Draw sprs using the same ones allocated for messages
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
              if (!IS_WALL_OR_MOB(tmap[newpos], newpos)) {
                mobwalk(mob - 1, dir);
              } else {
                mobbump(mob - 1, dir);
              }
            }
            mob_stun[mob - 1] = mob_vis[mob - 1] = 1;
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
      hide_sprite(num_sprs);
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
      shadow_OAM[i].x = spr_x[i] >> 8;
      shadow_OAM[i].y = spr_y[i] >> 8;
      shadow_OAM[i].tile = spr_anim_frame[i];
      shadow_OAM[i].prop = spr_prop[i];
      ++i;
    }
  }

  // Draw the targeting arrow (also using sprites 0 through 3)
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

        *next_sprite++ = y;
        *next_sprite++ = x;
        *next_sprite++ = TILE_ARROW_L + dir;
        *next_sprite++ = 0;
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
          set_tile_range_during_vbl(
              addr, pick_type_desc_tile + pick_type_desc_start[pick][i], len);
        } else {
          *mob_tile_code_ptr++ = 0x21; // ld hl, addr
          *mob_tile_code_ptr++ = addr & 0xff;
          *mob_tile_code_ptr++ = addr >> 8;
        }

        // Fill the rest of the row with 0
        u8* dst = mob_tile_code_ptr;
        *dst++ = 0xaf; // xor a
        while (len < INV_ROW_LEN) {
          *dst++ = 0x22; // ld (hl+), a
          ++len;
        }
        mob_last_tile_addr = addr + INV_ROW_LEN;
        mob_last_tile_val = 0;
        mob_tile_code_ptr = dst;
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

void addmob(MobType type, u8 pos) NONBANKED {
  mob_type[num_mobs] = type;
  mob_anim_frame[num_mobs] = mob_type_anim_start[type];
  mob_anim_timer[num_mobs] = 1;
  mob_anim_speed[num_mobs] = mob_type_anim_speed[type];
  mob_pos[num_mobs] = pos;
  mob_x[num_mobs] = POS_TO_X(pos);
  mob_y[num_mobs] = POS_TO_Y(pos);
  mob_clear_tile[num_mobs] = 0;
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

void delmob(u8 index) NONBANKED {  // XXX: only used in bank 1
  mobmap[mob_pos[index]] = 0;

  // If this mob was the key mob, then there is no more key mob.
  if (index + 1 == key_mob) {
    key_mob = 0;
  } else if (num_mobs == key_mob) {
    // If the last mob was the key mob, update it to index
    key_mob = index + 1;
  }

  --num_mobs;
  if (index != num_mobs) {
    mob_type[index] = mob_type[num_mobs];
    mob_anim_frame[index] = mob_anim_frame[num_mobs];
    mob_anim_timer[index] = mob_anim_timer[num_mobs];
    mob_anim_speed[index] = mob_anim_speed[num_mobs];
    mob_pos[index] = mob_pos[num_mobs];
    mob_x[index] = mob_x[num_mobs];
    mob_y[index] = mob_y[num_mobs];
    mob_dx[index] = mob_dx[num_mobs];
    mob_dy[index] = mob_dy[num_mobs];
    mob_clear_tile[index] = mob_clear_tile[num_mobs];
    mob_clear_pos[index] = mob_clear_pos[num_mobs];
    mob_move_timer[index] = mob_move_timer[num_mobs];
    mob_anim_state[index] = mob_anim_state[num_mobs];
    mob_flip[index] = mob_flip[num_mobs];
    mob_task[index] = mob_task[num_mobs];
    mob_target_pos[index] = mob_target_pos[num_mobs];
    mob_ai_cool[index] = mob_ai_cool[num_mobs];
    mob_active[index] = mob_active[num_mobs];
    mob_charge[index] = mob_charge[num_mobs];
    mob_hp[index] = mob_hp[num_mobs];
    mob_flash[index] = mob_flash[num_mobs];
    mob_stun[index] = mob_stun[num_mobs];
    mob_trigger[index] = mob_trigger[num_mobs];
    mob_vis[index] = mob_vis[num_mobs];
    mobmap[mob_pos[index]] = index + 1;
  }
}

void addpick(PickupType type, u8 pos) NONBANKED {
  pick_type[num_picks] = type;
  pick_anim_frame[num_picks] = pick_type_anim_start[type];
  pick_anim_timer[num_picks] = 1;
  pick_pos[num_picks] = pos;
  pick_move_timer[num_picks] = 0;
  ++num_picks;
  pickmap[pos] = num_picks; // index+1
}

void delpick(u8 index) NONBANKED {  // XXX: only used in bank 1
  pickmap[pick_pos[index]] = 0;
  --num_picks;
  if (index != num_picks) {
    pick_type[index] = pick_type[num_picks];
    pick_anim_frame[index] = pick_anim_frame[num_picks];
    pick_anim_timer[index] = pick_anim_timer[num_picks];
    pick_pos[index] = pick_pos[num_picks];
    pick_x[index] = pick_x[num_picks];
    pick_y[index] = pick_y[num_picks];
    pick_dx[index] = pick_dx[num_picks];
    pick_dy[index] = pick_dy[num_picks];
    pick_move_timer[index] = pick_move_timer[num_picks];
    pickmap[pick_pos[index]] = index + 1;
  }
}

u8 dropspot(u8 pos) {
  u8 i = 0, newpos;
  do {
    if ((newpos = is_valid(pos, drop_diff[i])) &&
        !(IS_SMARTMOB(tmap[newpos], newpos) || pickmap[newpos])) {
      return newpos;
    }
  } while (++i);
  return 0;
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

void adddeadmob(u8 index) {
  dmob_x[num_dead_mobs] = mob_x[index];
  dmob_y[num_dead_mobs] = mob_y[index];
  dmob_tile[num_dead_mobs] =
      mob_type_anim_frames[mob_anim_frame[index]] - TILE_MOB_FLASH_DIFF;
  dmob_prop[num_dead_mobs] = S_PALETTE | (mob_flip[index] ? S_FLIPX : 0);
  dmob_timer[num_dead_mobs] = DEAD_MOB_FRAMES;
  ++num_dead_mobs;
}

u8 addspr(u8 speed, u16 x, u16 y, u16 dx, u16 dy, u8 drag, u8 timer, u8 prop) {
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

void begin_tile_anim(void) NONBANKED {
  u16 addr = (u16)&tile_inc;
  // Initialize the first byte to ret, in case the interrupt handler is called
  // before we're finished.
  tile_code[0] = 0xc9; // ret (will be changed to push af)
  tile_code[1] = 0xc5; // push bc
  tile_code[2] = 0xfa; // ld a, (NNNN)
  tile_code[3] = (u8)addr;
  tile_code[4] = addr >> 8;
  tile_code[5] = 0x47; // ld b, a
  tile_code_ptr = tile_code + 6;
  last_tile_addr = 0;
  last_tile_val = 0;
}

void end_tile_anim(void) NONBANKED {
  u8 *ptr = tile_code_ptr;
  *ptr++ = 0xc1; // pop bc
  *ptr++ = 0xf1; // pop af
  *ptr = 0xc9;   // ret

  // Fix the first byte to push af, so the code can be executed
  tile_code[0] = 0xf5; // push af
}

void add_tile_anim(u8 pos, u8 tile) NONBANKED {
  u8* ptr = tile_code_ptr;
  u16 addr = POS_TO_ADDR(pos);
  if ((addr >> 8) != (last_tile_addr >> 8)) {
    *ptr++ = 0x21; // ld hl, NNNN
    *ptr++ = (u8)addr;
    *ptr++ = addr >> 8;
  } else {
    *ptr++ = 0x2e; // ld l, NN
    *ptr++ = (u8)addr;
  }
  last_tile_addr = addr;
  if (tile != last_tile_val) {
    *ptr++ = 0x3e; // ld a, NN
    *ptr++ = tile;
    *ptr++ = 0x80; // add b
    last_tile_val = tile;
  }
  *ptr++ = 0x77; // ld (hl), a
  tile_code_ptr = ptr;
}

void nop_saw_anim(u8 pos) {
  // Only nop out the memory write
  tile_code[sawmap[pos]] = 0;
}

const u8 randmask[] = {
  0,0,
  1,
  3,3,
  7,7,7,7,
  15,15,15,15,15,15,15,15,
  31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,
  63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,
  63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,
  127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,
  127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,
  127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,
  127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
};

u8 randint(u8 mx) NONBANKED {
  if (mx == 0) { return 0; }
  u8 result;
  do {
    result = xrnd() & randmask[mx];
  } while (result >= mx);
  return result;
}

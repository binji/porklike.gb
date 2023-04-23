#include <gb/gb.h>
#include <gb/gbdecompress.h>
#include <string.h>

#include "gameplay.h"

#include "ai.h"
#include "animate.h"
#include "counter.h"
#include "float.h"
#include "gameover.h"
#include "inventory.h"
#include "joy.h"
#include "mob.h"
#include "msg.h"
#include "palette.h"
#include "pickup.h"
#include "rand.h"
#include "sound.h"
#include "spr.h"
#include "sprite.h"
#include "targeting.h"

#pragma bank 1

#include "tilebg.h"

#define MSG_KEY 18
#define MSG_KEY_Y 80

#define TILE_VOID_EXIT_DIFF 0x35
#define TILE_VOID_OPEN_DIFF 3

// Sprite tiles
#define TILE_SHOT 0x1
#define TILE_ROPE_TIP_V 0x2
#define TILE_ROPE_TIP_H 0x3
#define TILE_ROPE_TAIL_H 0x4
#define TILE_ROPE_TAIL_V 0xd
#define TILE_SPIN 0x10

#define SPIN_SPR_FRAMES 12
#define SPIN_SPR_ANIM_SPEED 3

#define RECOVER_MOVES 30

#define REAPER_AWAKENS_STEPS 100
#define MSG_REAPER 0
#define MSG_REAPER_Y 83

#define MSG_WURSTCHAIN_PROP_LIT 5
#define MSG_WURSTCHAIN_PROP_DIM (S_PALETTE | 4)

#define TILE_BOOM 0x05

#define TILE_PLAYER_HIT_DMG_DIFF 0x5f

#define GAMEOVER_FRAMES 70
#define MOB_FLASH_FRAMES 20

#define SIGHT_DIFF_COUNT 57
#define SIGHT_DIFF_BLIND_COUNT 5

#define IS_UNSPECIAL_WALL_TILE(tile)                                           \
  ((flags_bin[tile] & 0b00000011) == 0b00000001)

static void do_turn(void);
static u8 pass_turn(void);
static void move_player(void);
static void use_pickup(void);
static u8 shoot(u8 pos, u8 hit, u8 tile, u8 prop);
static u8 shoot_dist(u8 pos, u8 hit);
static u8 rope(u8 from, u8 to);
static void update_wall_face(u8 pos);
static void update_tile(u8 pos, u8 tile);

static void unfog_tile(u8 pos);
static void unfog_center(u8 pos);
static void unfog_neighbors(u8 pos);
static void sight_blind(void);
static void nop_saw_anim(u8 pos);

extern const u8 boom_spr_speed[];
extern const u8 boom_spr_anim_speed[];
extern const u16 boom_spr_x[];
extern const u16 boom_spr_y[];
extern const u16 boom_spr_dx[];
extern const u16 boom_spr_dy[];

extern const u8 float_dmg[];

extern const u8 drop_diff[];

extern const u8 sightdiff[];
extern const u8 sightskip[];
extern const u8 sightdiffblind[];


// TODO: move to its own file
void mapgen(void);

u8 doupdatemap, dofadeout, doloadfloor, donextfloor, doblind, dosight;

Turn turn;
u8 noturn;

u8 recover; // how long until recovering from blind
u16 steps;
u8 num_keys;

Counter st_floor;
Counter st_steps;
Counter st_kills;
Counter st_recover;

u8 wurstchain;

void clear_bkg(void) {
  init_bkg(0);
#ifdef CGB_SUPPORT
  if (_cpu == CGB_TYPE) {
    VBK_REG = 1;
    init_bkg(0);
    VBK_REG = 0;
  }
#endif
}

void gameplay_init(void) {
  animate_init();

  // Reset scroll registers
  move_bkg(240, 0);
  // Reload bg tiles
  gb_decompress_bkg_data(0, tilebg_bin);
  clear_bkg();

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

  recover = 0;
  floor = 0;
  counter_zero(&st_floor);
  counter_zero(&st_steps);
  counter_zero(&st_kills);
  joy_init();
}

void gameplay_update(void) NONBANKED {
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
    clear_bkg(); // clear before enabling vblank
    joy_action = 0;
    IE_REG |= VBL_IFLAG;
    animate_end();
    if (floor == 0 && wurstchain) {
      msg_wurstchain(wurstchain, MSG_WURSTCHAIN_PROP_LIT);
    }
    doupdatemap = 1;
    // Update animations once so the mobs/pickups/etc. are shown on fadein.
    animate_begin();
    animate();
    animate_end();
    pal_fadein();
    animate_begin();
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
    if (!animate())
      break;

    if (pass_turn()) {
      if (!ai_run_tasks())
        continue;
      // AI took a step, so do 1 round of animation
      animate();
    }
    break;
  }

  inv_animate();
  animate_end();
  pal_update();
}

void do_turn(void) {
  if (inv_anim_up) {
    inv_update();
  } else if (turn == TURN_PLAYER) {
    move_player();
  }
}

u8 pass_turn(void) {
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
      return 1;
    }
    break;

  case TURN_AI:
    turn = TURN_WEEDS;
    return 1;

  case TURN_WEEDS:
    turn = TURN_PLAYER;
    if (floor && steps == REAPER_AWAKENS_STEPS) {
      ++steps; // Make sure to only spawn reaper once per floor
      addmob(MOB_TYPE_REAPER, dropspot(startpos));
      mob_active[num_mobs - 1] = 1;
      msg_show(MSG_REAPER, MSG_REAPER_Y);
      sfx(SFX_REAPER);
    }
    if (doblind) {
      // Update floor to display recover count instead
      inv_display_blind();
      float_blind();
      doblind = 0;
    } else if (recover) {
      if (--recover == 0) {
        // Set back to FLOOR #
        inv_display_floor();
        dosight = 1;
      } else {
        inv_decrement_recover();
      }
    }
    break;
  }

  return 0;
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
                    inv_update_keys();
                    dosight = 1;
                    sfx(SFX_BUMP_TILE);
                    goto dobump;
                  } else {
                    msg_show(MSG_KEY, MSG_KEY_Y);
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
        inv_open();
        is_targeting = 0;
        sfx(SFX_BACK);
      } else if (newjoy & J_A) {
        use_pickup();
        turn = TURN_PLAYER_MOVED;
        sfx(SFX_OK);
      }
    } else if (joy_action & J_A) {
      inv_open();
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
          spr = spr_add(SPIN_SPR_ANIM_SPEED + dir, x, y, dirx2[dir] << 8,
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

  inv_use_pickup();

  mob_trigger[PLAYER_MOB] = 1;
  dosight = 1;
  is_targeting = 0;
}

u8 shoot(u8 pos, u8 hit, u8 tile, u8 prop) {
  u8 spr = spr_add(255, POS_TO_X(pos) << 8, POS_TO_Y(pos) << 8,
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
    u8 spr = spr_add(255, x, y, dx, dy, 0, timer, prop);
    spr_type[spr] = SPR_TYPE_NONE;
    spr_anim_frame[spr] = tail;
    dx += ddx;
    dy += ddy;
    pos = POS_DIR(pos, target_dir);
  } while(pos != to);

  return shoot(from, to, tip, prop);
}

void trigger_step(u8 mob) {
  u8 pos, tile, ptype, pindex, teleported;

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
          inv_update_hp();
        }
        sound = SFX_HEART;
      } else if (ptype == PICKUP_TYPE_KEY) {
        if (num_keys < 9) {
          ++num_keys;
          inv_update_keys();
        }
      } else {
        if (!inv_acquire_pickup(ptype)) {
          // No room, display "full!"
          ptype = PICKUP_TYPE_FULL;
          sound = SFX_OOPS;
          goto done;
        }
      }

      delpick(pindex);

    done:
      sfx(sound);
      float_pickup(ptype);
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

void sight(void) NONBANKED {
  u8 index, pos;
  clear_map(mobsightmap);

  if (recover) {
    sight_blind();
    return;
  }

  for (index = 0; index < SIGHT_DIFF_COUNT; ++index) {
    if (is_new_pos_valid(mob_pos[PLAYER_MOB], sightdiff[index])) {
      mobsightmap[pos = pos_result] = 1;
      if (fogmap[pos]) unfog_center(pos);

      if (!IS_OPAQUE_POS(pos)) {
        unfog_neighbors(pos);
        continue;
      }
    }
    index += sightskip[index];
  }
}

void sight_blind(void) NONBANKED {
  u8 index, pos;
  for (index = 0; index < SIGHT_DIFF_BLIND_COUNT; ++index) {
    if (is_new_pos_valid(mob_pos[PLAYER_MOB], sightdiffblind[index])) {
      if (fogmap[pos = pos_result]) unfog_center(pos);

      if (!IS_OPAQUE_POS(pos)) {
        unfog_neighbors(pos);
      }
    }
  }

  for (index = 0; index < SIGHT_DIFF_COUNT; ++index) {
    if (is_new_pos_valid(mob_pos[PLAYER_MOB], sightdiff[index])) {
      mobsightmap[pos = pos_result] = 1;
      if (!IS_OPAQUE_POS(pos)) {
        continue;
      }
    }
    index += sightskip[index];
  }
}

void blind(void) {
  if (!recover) {
    recover = RECOVER_MOVES;

    // Reset fogmap, dtmap and sawmap
    memset(fogmap, 1, sizeof(fogmap));
    clear_map(dtmap);
    clear_map(sawmap);

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
      if (floor == 0 && wurstchain) {
        msg_wurstchain(wurstchain, MSG_WURSTCHAIN_PROP_DIM);
        // Clear out wurstchain in SRAM immediately; if you turn it off you're
        // back to 0.
        sram_update_wurstchain(0);
      }
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
        spr = spr_add(boom_spr_anim_speed[i], x + boom_spr_x[i],
                      y + boom_spr_y[i], boom_spr_dx[i], boom_spr_dy[i], 1,
                      boom_spr_speed[i], 0);
        spr_type[spr] = SPR_TYPE_BOOM;
        spr_anim_frame[spr] = TILE_BOOM;
      }
    }

    sfx(sound);
  } else {
    if (index == PLAYER_MOB) {
      float_add(pos, float_dmg[dmg] + TILE_PLAYER_HIT_DMG_DIFF, 2);
    } else {
      float_add(pos, float_dmg[dmg], 1);
    }

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
        inv_update_hp();
        gameover_timer = GAMEOVER_FRAMES;
      } else {
        // Killing a reaper restores the your health to 5 and drops a free key
        if (mtype == MOB_TYPE_REAPER) {
          if (mob_hp[PLAYER_MOB] < 5) {
            mob_hp[PLAYER_MOB] = 5;
            inv_update_hp();
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
        inv_update_hp();
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

void unfog_center(u8 pos) NONBANKED {
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

void unfog_neighbors(u8 pos) NONBANKED {
  u8 valid = validmap[pos];
  u8 adjpos = POS_L(pos);
  if (fogmap[adjpos] && IS_WALL_POS(adjpos) && (valid & VALID_L)) {
    unfog_tile(adjpos);
  }
  adjpos += 2; // L -> R
  if (fogmap[adjpos] && IS_WALL_POS(adjpos) && (valid & VALID_R)) {
    unfog_tile(adjpos);
  }
  adjpos -= 17; // R -> U
  if (fogmap[adjpos] && IS_WALL_POS(adjpos) && (valid & VALID_U)) {
    unfog_tile(adjpos);
  }
  adjpos += 32; // U -> D
  if (fogmap[adjpos] && IS_WALL_POS(adjpos) && (valid & VALID_D)) {
    unfog_tile(adjpos);
  }
}

void trigger_spr(SprType type, u8 trigger_val) {
  u8 dir, stun;
  switch (type) {
  case SPR_TYPE_SPIN:
    stun = 0;
    goto hit;

  case SPR_TYPE_BOLT:
    stun = 1;
    goto hit;

  hit:
    hitpos(trigger_val, 1, stun);
    break;

  case SPR_TYPE_HOOK:
    dir = invdir[target_dir];
    goto push;

  case SPR_TYPE_PUSH:
    dir = target_dir;
    goto push;

  push: {
    u8 mob = trigger_val;
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
    mobhop(PLAYER_MOB, trigger_val);
    break;
  }
}

void nop_saw_anim(u8 pos) {
  u8 index = sawmap[pos];
  pos = *--anim_tile_ptr;
  if (sawmap[pos]) { sawmap[pos] = index; }
  anim_tiles[index] = pos;
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

void sram_init(void) {
  ENABLE_RAM;
  // Check SRAM.
  if (!(_SRAM[0] == 'p' && _SRAM[1] == 'o' && _SRAM[2] == 'r' &&
        _SRAM[3] == 'k' && _SRAM[4] < MAX_WURSTCHAIN)) {
    _SRAM[0] = 'p';
    _SRAM[1] = 'o';
    _SRAM[2] = 'r';
    _SRAM[3] = 'k';
    _SRAM[4] = 0;
  } else {
    wurstchain = _SRAM[4];
  }
  DISABLE_RAM;
}

void sram_update_wurstchain(u8 value) {
  ENABLE_RAM;
  _SRAM[4] = value;
  DISABLE_RAM;
}

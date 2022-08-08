#include <gb/gb.h>

#include "ai.h"
#include "float.h"
#include "gameplay.h"
#include "joy.h"
#include "mob.h"
#include "rand.h"
#include "sound.h"

#define QUEEN_CHARGE_MOVES 2
#define AI_COOL_MOVES 8

#pragma bank 1

u8 ai_run_tasks(void) {
  u8 ai_acted = 1;
  u8 i;
  for (i = 0; i < num_mobs; ++i) {
    if ((mob_type[i] == MOB_TYPE_WEED) == (turn == TURN_WEEDS)) {
      ai_acted |= ai_run_mob_task(i);
    }
  }
  return ai_acted;
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
          float_add(pos, FLOAT_FOUND, 0);
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
          float_add(pos, FLOAT_LOST, 0);
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
    float_add(mob_pos[index], FLOAT_LOST, 0);
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


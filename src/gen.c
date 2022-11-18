#ifdef __SDCC
#include <gb/gb.h>
#endif
#include <string.h>

#include "animate.h"
#include "gameplay.h"
#include "main.h"
#include "mob.h"
#include "pickup.h"
#include "rand.h"

#include "map.c"

#pragma bank 3

#define MAX_ROOMS 4
#define MAX_DS_SET 52
#define MAX_VOIDS 10

#define MIN_CARVECUT_DIST 21

#define NUM_GATE_MAPS 14

#define MASK_UL  0b11111110
#define MASK_DL  0b11111101
#define MASK_DR  0b11111011
#define MASK_UR  0b11110111
#define MASK_D   0b11101111
#define MASK_U   0b11011111
#define MASK_R   0b10111111
#define MASK_L   0b01111111

#define TILE_WALLSIG_BASE 0xf
#define TILE_CRACKED_DIFF 0x34

#define IS_DOOR(pos) sigmatch((pos), doorsig, doormask)
#define CAN_CARVE(pos) sigmatch((pos), carvesig, carvemask)
#define IS_FREESTANDING(pos) sigmatch((pos), freesig, freemask)

extern Map cands;
extern Map tempmap;
extern Map sigmap;

typedef enum RoomKind {
  ROOM_KIND_VASE,
  ROOM_KIND_DIRT,
  ROOM_KIND_CARPET,
  ROOM_KIND_TORCH,
  ROOM_KIND_PLANT,
  NUM_ROOM_KINDS,
} RoomKind;

// floor(35 / x) for x in [3,10]
const u8 other_dim[] = {11, 8, 7, 5, 5, 4, 3, 3};

const u8 carvesig[] = {255, 223, 127, 191, 239, 0};
const u8 carvemask[] = {0, 9, 3, 12, 6};
const u8 doorsig[] = {207, 63, 0};
const u8 doormask[] = {15, 15};

const u8 freesig[] = {8, 4, 2, 1, 22, 76, 41, 131, 171, 109, 94, 151, 0};
const u8 freemask[] = {8, 4, 2, 1, 6, 12, 9, 3, 10, 5, 10, 5};

const u8 wallsig[] = {
    251, 239, 253, 95,  159, 91,  31,  157, 115, 217, 241, 248,
    219, 189, 231, 123, 191, 15,  127, 111, 175, 79,  240, 143,
    230, 188, 242, 244, 119, 238, 190, 221, 247, 223, 254, 207,
    63,  103, 47,  174, 245, 250, 243, 249, 246, 252, 0,
};
const u8 wallmask[] = {
    0,  6,  0, 11, 13, 11, 15, 13, 3, 9,  0, 0, 9, 12, 6,  3,
    12, 15, 3, 7,  14, 15, 0,  15, 6, 12, 0, 0, 3, 6,  12, 9,
    0,  9,  0, 15, 15, 7,  15, 14, 0, 0,  0, 0, 0, 0,
};

const u8 void_tiles[] = {TILE_VOID1, TILE_VOID1, TILE_VOID1, TILE_VOID1,
                         TILE_VOID1, TILE_VOID1, TILE_VOID2};
const u8 plant_tiles[] = {TILE_PLANT1, TILE_PLANT2, TILE_PLANT3};

const u8 vase_mobs[] = {0, 0, 0, 0, 0, 0, MOB_TYPE_VASE1, MOB_TYPE_VASE2};

const u8 plant_mobs[] = {MOB_TYPE_WEED, MOB_TYPE_WEED, MOB_TYPE_BOMB};

const u8 mob_plan[] = {
    // floor 1
    MOB_TYPE_SLIME, MOB_TYPE_SLIME, MOB_TYPE_SLIME, MOB_TYPE_SLIME,
    MOB_TYPE_SLIME,
    // floor 2
    MOB_TYPE_SLIME, MOB_TYPE_SLIME, MOB_TYPE_QUEEN, MOB_TYPE_SLIME,
    MOB_TYPE_SLIME, MOB_TYPE_QUEEN,
    // floor 3
    MOB_TYPE_SLIME, MOB_TYPE_SLIME, MOB_TYPE_QUEEN, MOB_TYPE_SLIME,
    MOB_TYPE_SLIME, MOB_TYPE_QUEEN, MOB_TYPE_SLIME,
    // floor 4
    MOB_TYPE_SCORPION, MOB_TYPE_SCORPION, MOB_TYPE_SCORPION, MOB_TYPE_HULK,
    MOB_TYPE_SCORPION, MOB_TYPE_SCORPION, MOB_TYPE_SCORPION, MOB_TYPE_HULK,
    // floor 5
    MOB_TYPE_SCORPION, MOB_TYPE_SCORPION, MOB_TYPE_HULK, MOB_TYPE_SCORPION,
    MOB_TYPE_SCORPION, MOB_TYPE_HULK, MOB_TYPE_SCORPION, MOB_TYPE_SCORPION,
    MOB_TYPE_HULK,
    // floor 6
    MOB_TYPE_HULK, MOB_TYPE_SCORPION, MOB_TYPE_HULK, MOB_TYPE_SCORPION,
    MOB_TYPE_SCORPION, MOB_TYPE_HULK, MOB_TYPE_SCORPION, MOB_TYPE_SCORPION,
    MOB_TYPE_HULK,
    // floor 7
    MOB_TYPE_GHOST, MOB_TYPE_GHOST, MOB_TYPE_GHOST, MOB_TYPE_KONG,
    MOB_TYPE_GHOST, MOB_TYPE_GHOST, MOB_TYPE_GHOST, MOB_TYPE_KONG,
    MOB_TYPE_GHOST,
    // floor 8
    MOB_TYPE_GHOST, MOB_TYPE_GHOST, MOB_TYPE_KONG, MOB_TYPE_GHOST,
    MOB_TYPE_GHOST, MOB_TYPE_KONG, MOB_TYPE_GHOST, MOB_TYPE_GHOST,
    MOB_TYPE_KONG,
    // floor 9
    MOB_TYPE_GHOST, MOB_TYPE_KONG, MOB_TYPE_GHOST, MOB_TYPE_KONG,
    MOB_TYPE_GHOST, MOB_TYPE_KONG, MOB_TYPE_GHOST, MOB_TYPE_KONG,
    MOB_TYPE_GHOST};

const u8 mob_plan_floor[] = {
    0,  // floor 0
    0,  // floor 1
    5,  // floor 2
    11, // floor 3
    18, // floor 4
    26, // floor 5
    35, // floor 6
    44, // floor 7
    53, // floor 8
    62, // floor 9
    70, // total
};

const u8 mob_type_key[] = {
    0, // player
    1, // slime
    1, // queen
    1, // scorpion
    1, // hulk
    1, // ghost
    1, // kong
    0, // reaper
    0, // weed
    0, // bomb
    0, // vase1
    0, // vase2
    0, // chest
    0, // heart chest
};

void mapgeninit(void);
void initdtmap(void);
void initflagmap(void);
void copymap(u8 index);
void roomgen(void);
void mazeworms(void);
void update_carve1(u8 pos);
u8 nexttoroom4(u8 pos, u8 valid);
u8 nexttoroom8(u8 pos, u8 valid);
void digworm(u8 pos);
u8 carvedoors(void);
void update_door1(u8 pos);
void carvecuts(void);
u8 is_valid_cut(u8 from, u8 to);
void calcdist(u8 pos);
void startend(void);
u8 startscore(u8 pos);
void fillends(void);
void update_fill1(u8 pos);
void prettywalls(void);
void voids(void);
void chests(void);
void decoration(void);
void spawnmobs(void);
u8 no_mob_neighbors(u8 pos);
void mapset(u8* pos, u8 w, u8 h, u8 val);
void sigrect_empty(u8 pos, u8 w, u8 h);
void sigempty(u8 pos);
void sigwall(u8 pos);
u8 sigmatch(u16 pos, const u8* sig, const u8* mask);
u8 ds_union(u8 x, u8 y);
u8 ds_find(u8 id);
u8 ds_find_pos(u8 pos);
u8 getpos(u8 cand);

u8 room_pos[MAX_ROOMS];
u8 room_w[MAX_ROOMS];
u8 room_h[MAX_ROOMS];
u8 room_avoid[MAX_ROOMS]; // ** only used during mapgen
u8 num_rooms;

u8 start_room;
u8 startpos;

u8 num_cands;

// The following data structures is used for rooms as well as union-find
// algorithm; see https://en.wikipedia.org/wiki/Disjoint-set_data_structure
Map ds_sets;                   // **only used during mapgen
u8 ds_parents[MAX_DS_SET];     // **only used during mapgen
u8 ds_sizes[MAX_DS_SET];       // **only used during mapgen
u8 ds_nextid;                  // **only used during mapgen
u8 ds_num_sets;                // **only used during mapgen
u8 dogate;                     // **only used during mapgen
u8 endpos, gatepos, heartpos;  // **only used during mapgen

u8 void_num_tiles[MAX_VOIDS]; // Number of tiles in a void region
u8 void_exit[MAX_VOIDS];      // Exit tile for a given void region
u8 num_voids;

u16 floor_seed;

void mapgen(void) {
  floor_seed = xrnd_seed;
  u8 fog;
redo:
  anim_tile_ptr = anim_tiles;
  num_picks = 0;
  num_mobs = 1;
  num_dead_mobs = 0;
  steps = 0;
  mapgeninit();
  if (floor == 0) {
    fog = 0;
    copymap(0);
    addmob(MOB_TYPE_CHEST, 119);  // x=7,y=7
  } else if (floor == 10) {
    fog = 0;
    copymap(1);
  } else {
    fog = 1;
    roomgen();
    mazeworms();
    if (!carvedoors()) { goto redo; }
    carvecuts();
    startend();
    fillends();
    prettywalls();
    voids();
    chests();
    decoration();
    spawnmobs();
  }
  memset(fogmap, fog, sizeof(fogmap));
  initflagmap();
  sight();
  initdtmap();
}

void mapgeninit(void) {
  memset(dirtymap, 0, sizeof(dirtymap));
  memset(roommap, 0, sizeof(roommap));
  memset(mobmap, 0, sizeof(mobmap));
  memset(pickmap, 0, sizeof(pickmap));
  memset(sigmap, 255, sizeof(sigmap));
  memset(ds_sets, 0, sizeof(ds_sets));
  memset(sawmap, 0, sizeof(sawmap));

  dogate = 0;

  // Initialize disjoint set
  ds_nextid = 0;
  ds_parents[0] = 0;
  ds_num_sets = 0;

  num_rooms = 0;
  num_voids = 0;
  start_room = 0;

  mob_anim_timer[PLAYER_MOB] = 1;
  mob_anim_speed[num_mobs] = mob_type_anim_speed[MOB_TYPE_PLAYER];
  mob_move_timer[PLAYER_MOB] = 0;
  mob_flip[PLAYER_MOB] = 0;
}

void roomgen(void) {
  u8 failmax = 5, maxwidth = 10, maxheight = 10, maxh;
  u8 sig, valid, walkable, val, w, h, pos;

  dogate = floor == 3 || floor == 6 || floor == 9;

  if (dogate) {
    // Pick a random gate map
    copymap(2 + randint(NUM_GATE_MAPS));
  } else {
    // Start with a completely empty map (all walls)
    memset(tmap, TILE_WALL, sizeof(tmap));
  }

  do {
    // Pick width and height for room.
    w = 3 + randint(maxwidth - 2);
    maxh = other_dim[w - 3];
    if (maxh > maxheight) maxh = maxheight;
    h = 3 + randint(maxh - 2);

    // Update width/height maps
    //
    // The basic idea is to start in the lower-right corner and work backward.
    // There are two maps; one that tracks whether a room of this width will
    // fit, and the other tracks whether a room of this height will fit. When
    // both values are 0, the room fits.
    //
    // The room must have a one tile border around it, except for when it is
    // adjacent to the map edge.
    num_cands = 0;
    pos = 255;
    do {
      valid = validmap[pos];
      sig = ~sigmap[pos];
      walkable = !IS_WALL_TILE(tmap[pos]);
      if (walkable || (sig & VALID_L_OR_UL)) {
        // This tile is either in a room, or on the right or lower border of
        // the room.
        val = h + 1;
      } else if (valid & VALID_D) {
        // This tile is above a room or the bottom edge; we subtract one from
        // the value below to track whether it is far enough away to fit a room
        // of height `h`.
        val = tempmap[POS_D(pos)];
        if (val) { --val; }
      } else {
        // This tile is on the bottom edge of the map.
        val = h - 1;
      }
      tempmap[pos] = val;

      if (walkable || (sig & VALID_U_OR_UL)) {
        // This tile is either in a room, or on the right or lower border of
        // the room.
        val = w + 1;
      } else if (valid & VALID_R) {
        val = distmap[POS_R(pos)];
        if (val) {
          // This tile is to the left of a room or the right edge of the map.
          // We also update the "height" map in this case to extend out the
          // invalid region vertically. For example, if placing a 3x3 room,
          // where there is an existing room (marked with x):
          //
          // width map   height map   overlap
          // 000000012   000000000    0000000..
          // 000000012   011112220    0........
          // 000000012   022222220    0........
          // 000000012   033333330    0........
          // 0123xxxx2   0444xxxx0    0........
          // 0123xxxx2   0444xxxx0    0........
          // 0123xxxx2   1111xxxx1    .........
          // 000000012   222222222    .........
          //
          --val;
          tempmap[pos] = tempmap[POS_R(pos)];
        } else {
          val = 0;
          if (!tempmap[pos]) {
            // This is valid position, add it to the candidate list.
            cands[num_cands++] = pos;
          }
        }
      } else {
        val = w - 1;
      }
      distmap[pos] = val;
    } while(pos--);

    if (num_cands) {
      pos = cands[randint(num_cands)];

      room_pos[num_rooms] = pos;
      room_w[num_rooms] = w;
      room_h[num_rooms] = h;
      room_avoid[num_rooms] = 0;

      // Fill room tiles at x, y
      mapset(tmap + pos, w, h, 1);
      mapset(roommap + pos, w, h, num_rooms + 1);
      sigrect_empty(pos, w, h);

      // Update disjoint set regions; we know that they're disjoint so they
      // each get their own region id (i.e. the room number)
      mapset(ds_sets + pos, w, h, ++ds_nextid);
      ds_parents[ds_nextid] = ds_nextid;
      ds_sizes[ds_nextid] = w * h; // XXX multiply
      ++ds_num_sets;

      if (++num_rooms == 1) {
        maxwidth >>= 1;
        maxheight >>= 1;
      }
    } else {
      // cannot place room
      --failmax;
    }
  } while (failmax && num_rooms < MAX_ROOMS);
}

void copymap(u8 index) {
  const u8 *src;
  u8 *dst;
  u8 pos, valid, i, w, h;

  // Fill map with default tiles. TILE_NONE is used for floor0 and floor10, and
  // TILE_WALL is used for all gate maps
  memset(tmap, index < 2 ? TILE_NONE : TILE_WALL, 256);

  // Each premade map only has a small rectangular region to copy (map_w by
  // map_h tiles), and is offset by map_pos.
  src = map_bin + map_start[index];
  dst = tmap + map_pos[index];
  w = map_w[index];
  h = map_h[index];
  do {
    i = w;
    do {
      *dst++ = *src++;
    } while (--i);
    dst += MAP_WIDTH - w;
  } while (--h);

  // Update signature map for empty tiles
  pos = 0;
  do {
    switch (tmap[pos]) {
      case TILE_END:
        endpos = pos;
        goto empty;

      case TILE_START:
      case TILE_ENTRANCE:
        startpos = pos;
        mobmap[mob_pos[PLAYER_MOB]] = 0; // Clear old player location
        mob_pos[PLAYER_MOB] = pos;
        mobmap[pos] = PLAYER_MOB + 1;
        goto empty;

      case TILE_GATE:
        // Temporarily change to stairs so the empty spaces behind the gate
        // can be part of the spanning graph created during the carvedoors()
        // step.
        tmap[pos] = TILE_END;
        gatepos = pos;
        goto empty;

      case TILE_TORCH_LEFT:
      case TILE_TORCH_RIGHT:
        *anim_tile_ptr++ = pos;
        goto empty;

      case TILE_HEART:
        heartpos = pos;
        tmap[pos] = TILE_RUG;
        goto empty;

      empty:
      case TILE_CARPET:
      case TILE_EMPTY:
      case TILE_STEPS:
        // Update sigmap
        sigempty(pos);

        // Update disjoint set for the gate area
        ++ds_nextid;
        ++ds_num_sets;
        ds_sets[pos] = ds_nextid;
        ds_parents[ds_nextid] = ds_nextid;
        ds_sizes[ds_nextid] = 1;

        valid = validmap[pos];
        if (valid & VALID_L) { ds_union(ds_nextid, ds_sets[POS_L(pos)]); }
        if (valid & VALID_R) { ds_union(ds_nextid, ds_sets[POS_R(pos)]); }
        if (valid & VALID_U) { ds_union(ds_nextid, ds_sets[POS_U(pos)]); }
        if (valid & VALID_D) { ds_union(ds_nextid, ds_sets[POS_D(pos)]); }
        break;
    }
  } while (++pos);
}

void mazeworms(void) {
  u8 pos, cand, valid;

  memset(tempmap, 0, sizeof(tempmap));

  // Find all candidates for worm start position
  num_cands = 0;
  pos = 0;
  do {
    update_carve1(pos);
  } while(++pos);

  do {
    if (num_cands) {
      // Choose a random start, and calculate its offset
      cand = randint(num_cands);
      pos = 0;
      do {
        if (tempmap[pos] == 2 && cand-- == 0) { break; }
      } while(++pos);

      // Update the disjoint set with this new region.
      ++ds_nextid;
      ++ds_num_sets;
      ds_parents[ds_nextid] = ds_nextid;
      ds_sizes[ds_nextid] = 0;

      digworm(pos);

      // Only the first spot may be connected to another empty tile. No other
      // tile can be connect to another region (since it would not be
      // carvable). So we only need to merge regions here.
      valid = validmap[pos];
      if (valid & VALID_L) { ds_union(ds_nextid, ds_sets[POS_L(pos)]); }
      if (valid & VALID_R) { ds_union(ds_nextid, ds_sets[POS_R(pos)]); }
      if (valid & VALID_U) { ds_union(ds_nextid, ds_sets[POS_U(pos)]); }
      if (valid & VALID_D) { ds_union(ds_nextid, ds_sets[POS_D(pos)]); }
    }
  } while(num_cands > 1);
}

void update_carve1(u8 pos) {
  u8 tile = tmap[pos];
  u8 result;
  if (tile != TILE_FIXED_WALL && IS_WALL_TILE(tile) && CAN_CARVE(pos)) {
    result = 1;
    if (!nexttoroom4(pos, validmap[pos])) {
      ++result;
      if (tempmap[pos] != 2) { ++num_cands; }
    }
  } else {
    result = 0;
    if (tempmap[pos] == 2) { --num_cands; }
  }
  tempmap[pos] = result;
}

u8 nexttoroom4(u8 pos, u8 valid) {
  return ((valid & VALID_U) && roommap[POS_U(pos)]) ||
         ((valid & VALID_L) && roommap[POS_L(pos)]) ||
         ((valid & VALID_R) && roommap[POS_R(pos)]) ||
         ((valid & VALID_D) && roommap[POS_D(pos)]);
}

u8 nexttoroom8(u8 pos, u8 valid) {
  return nexttoroom4(pos, valid) ||
         ((valid & VALID_UL) && roommap[POS_UL(pos)]) ||
         ((valid & VALID_UR) && roommap[POS_UR(pos)]) ||
         ((valid & VALID_DL) && roommap[POS_DL(pos)]) ||
         ((valid & VALID_DR) && roommap[POS_DR(pos)]);
}

void digworm(u8 pos) {
  u8 dir, step, num_dirs, valid;

  // Pick a random direction
  dir = xrnd() & 3;
  step = 0;
  do {
    tmap[pos] = 1; // Set tile to empty.
    sigempty(pos); // Update neighbor signatures.

    // Remove this tile from the carvable map
    if (tempmap[pos] == 2) { --num_cands; }
    tempmap[pos] = 0;

    // Update neighbors
    valid = validmap[pos];
    if (valid & VALID_L) { update_carve1(POS_L(pos)); }
    if (valid & VALID_R) { update_carve1(POS_R(pos)); }
    if (valid & VALID_U) { update_carve1(POS_U(pos)); }
    if (valid & VALID_D) { update_carve1(POS_D(pos)); }
    if (valid & VALID_UL) { update_carve1(POS_UL(pos)); }
    if (valid & VALID_UR) { update_carve1(POS_UR(pos)); }
    if (valid & VALID_DL) { update_carve1(POS_DL(pos)); }
    if (valid & VALID_DR) { update_carve1(POS_DR(pos)); }

    // Update the disjoint set
    ds_sets[pos] = ds_nextid;
    ++ds_sizes[ds_nextid];

    // Continue in the current direction, unless we can't carve. Also randomly
    // change direction after 3 or more steps.
    if (!((valid & dirvalid[dir]) && tempmap[POS_DIR(pos, dir)]) ||
        (XRND_50_PERCENT() && step > 2)) {
      step = 0;
      num_dirs = 0;

      if ((valid & VALID_L) && tempmap[POS_L(pos)]) { cands[num_dirs++] = 0; }
      if ((valid & VALID_R) && tempmap[POS_R(pos)]) { cands[num_dirs++] = 1; }
      if ((valid & VALID_U) && tempmap[POS_U(pos)]) { cands[num_dirs++] = 2; }
      if ((valid & VALID_D) && tempmap[POS_D(pos)]) { cands[num_dirs++] = 3; }
      if (num_dirs == 0) return;
      dir = cands[randint(num_dirs)];
    }
    pos = POS_DIR(pos, dir);
    ++step;
  } while(1);
}

u8 carvedoors(void) {
  u8 pos, match, diff;

  memset(tempmap, 0, sizeof(tempmap));

  pos = 0;
  num_cands = 0;
  do {
    update_door1(pos);
  } while(++pos);

  do {
    // Pick a random door, and calculate its position.
    pos = getpos(randint(num_cands));

    // Merge the regions, if possible. They may be already part of the same
    // set, in which case they should not be joined.
    match = IS_DOOR(pos);
    diff = match == 1 ? 16 : 1; // 1: horizontal, 2: vertical
    if (ds_union(ds_sets[pos - diff], ds_sets[pos + diff])) {
      // Insert an empty tile to connect the regions
      ds_sets[pos] = ds_sets[pos + diff];
      tmap[pos] = 1;
      sigempty(pos); // Update neighbor signatures.

      // Update neighbors
      u8 valid = validmap[pos];
      if (valid & VALID_U) { update_door1(POS_U(pos)); }
      if (valid & VALID_L) { update_door1(POS_L(pos)); }
      if (valid & VALID_R) { update_door1(POS_R(pos)); }
      if (valid & VALID_D) { update_door1(POS_D(pos)); }
    }

    // Remove this tile from the carvable map
    --num_cands;
    tempmap[pos] = 0;
  } while (num_cands);

  // Make sure all rooms are connected, as well as the gate region, if any. If
  // not, we'll need to regenerate.
  u8 id = ds_find_pos(room_pos[0]);
  for (u8 i = 1; i < num_rooms; ++i) {
    if (ds_find_pos(room_pos[i]) != id) {
      return 0;
    }
  }

  return dogate ? id == ds_find_pos(gatepos) : 1;
}

void update_door1(u8 pos) {
  // A door connects two regions in the following patterns:
  //   * 0 *   * 1 *
  //   1   1   0   0
  //   * 0 *   * 1 *
  //
  // Where 1s are walls and 0s are not. The `match` index returns whether it is
  // a horizontal door or vertical. A door is only allowed between two regions
  // that are not already connected. We use the disjoint set to determine this
  // quickly below.
  u8 tile = tmap[pos];
  u8 result = tile != TILE_FIXED_WALL && IS_WALL_TILE(tile) && IS_DOOR(pos);
  if (tempmap[pos] != result) {
    if (result) {
      ++num_cands;
    } else {
      --num_cands;
    }
    tempmap[pos] = result;
  }
}

void carvecuts(void) {
  u8 pos, diff, match;
  u8 maxcuts = 3;

  num_cands = 0;
  pos = 0;
  do {
    update_door1(pos);
  } while(++pos);

  do {
    // Pick a random cut, and calculate its position.
    pos = getpos(randint(num_cands));

    // Calculate distance from one side of the door to the other.
    match = IS_DOOR(pos);
    diff = match == 1 ? 16 : 1; // 1: horizontal, 2: vertical

    // Connect the regions (creating a cycle) if the distance between the two
    // sides is > MIN_CARVECUT_DIST steps.
    if (is_valid_cut(pos + diff, pos - diff)) {
      tmap[pos] = 1;
      sigempty(pos); // Update neighbor signatures.
      --maxcuts;
    }

    // Remove this candidate
    tempmap[pos] = 0;
    --num_cands;
  } while (num_cands && maxcuts);
}

u8 is_valid_cut(u8 pos, u8 to) {
  u8 head, oldtail, newtail, sig, valid, dist, newpos;
  cands[head = 0] = pos;
  newtail = 1;

  memset(distmap, 0, sizeof(distmap));
  distmap[pos] = dist = 1;

  do {
    oldtail = newtail;
    ++dist;
    do {
      pos = cands[head++];
      if (pos == to) { return 0; }

      valid = validmap[pos];
      sig = ~sigmap[pos] & valid;
      if (!distmap[newpos = POS_L(pos)]) {
        if (valid & VALID_L) { distmap[newpos] = dist; }
        if (sig & VALID_L) { cands[newtail++] = newpos; }
      }
      if (!distmap[newpos = POS_R(pos)]) {
        if (valid & VALID_R) { distmap[newpos] = dist; }
        if (sig & VALID_R) { cands[newtail++] = newpos; }
      }
      if (!distmap[newpos = POS_U(pos)]) {
        if (valid & VALID_U) { distmap[newpos] = dist; }
        if (sig & VALID_U) { cands[newtail++] = newpos; }
      }
      if (!distmap[newpos = POS_D(pos)]) {
        if (valid & VALID_D) { distmap[newpos] = dist; }
        if (sig & VALID_D) { cands[newtail++] = newpos; }
      }
    } while (head != oldtail);
  } while (dist < MIN_CARVECUT_DIST && oldtail != newtail);

  // Distance is greater than MIN_CARVECUT_DIST
  return 1;
}

void calcdist(u8 pos) {
  u8 head, oldtail, newtail, sig, valid, dist, newpos;
  cands[head = 0] = pos;
  newtail = 1;

  memset(distmap, 0, sizeof(distmap));
  distmap[pos] = dist = 1;

  do {
    oldtail = newtail;
    ++dist;
    do {
      pos = cands[head++];
      valid = validmap[pos];
      sig = ~sigmap[pos] & valid;
      if (!distmap[newpos = POS_L(pos)]) {
        if (valid & VALID_L) { distmap[newpos] = dist; }
        if (sig & VALID_L) { cands[newtail++] = newpos; }
      }
      if (!distmap[newpos = POS_R(pos)]) {
        if (valid & VALID_R) { distmap[newpos] = dist; }
        if (sig & VALID_R) { cands[newtail++] = newpos; }
      }
      if (!distmap[newpos = POS_U(pos)]) {
        if (valid & VALID_U) { distmap[newpos] = dist; }
        if (sig & VALID_U) { cands[newtail++] = newpos; }
      }
      if (!distmap[newpos = POS_D(pos)]) {
        if (valid & VALID_D) { distmap[newpos] = dist; }
        if (sig & VALID_D) { cands[newtail++] = newpos; }
      }
    } while (head != oldtail);
  } while (oldtail != newtail);
}

void startend(void) {
  u8 pos, best, score;

  if (!dogate) {
    // Pick a random walkable location
    do {
      endpos = xrnd();
    } while (IS_WALL_TILE(tmap[endpos]));
  }

  calcdist(endpos);

  startpos = 0;
  best = 0;
  // Find the furthest point from endpos.
  pos = 0;
  do {
    if (distmap[pos] > best && !IS_WALL_TILE(tmap[pos])) {
      startpos = pos;
      best = distmap[pos];
    }
  } while(++pos);

  calcdist(startpos);

  if (!dogate) {
    // Now pick the furthest, freestanding location in a room from the startpos.
    pos = 0;
    best = 0;
    do {
      if (distmap[pos] > best && roommap[pos] && IS_FREESTANDING(pos)) {
        endpos = pos;
        best = distmap[pos];
      }
    } while(++pos);
  }

  // Finally pick the closest point that has the best startscore.
  pos = 0;
  best = 255;
  do {
    score = startscore(pos);
    if (distmap[pos] && score) {
      score = distmap[pos] + score;
      if (score < best) {
        best = score;
        startpos = pos;
      }
    }
  } while(++pos);

  if (roommap[startpos]) {
    start_room = roommap[startpos];
  }

  tmap[startpos] = TILE_START;
  tmap[endpos] = TILE_END;
  sigempty(startpos);
  sigempty(endpos);
  mob_pos[PLAYER_MOB] = startpos;
  mobmap[startpos] = PLAYER_MOB + 1;

  if (dogate) {
    tmap[gatepos] = TILE_GATE;
    if (floor < 9) {
      addpick(PICKUP_TYPE_HEART, heartpos);
    }
  }
}

u8 startscore(u8 pos) {
  u8 score = IS_FREESTANDING(pos);
  // If the position is not in a room...
  if (!roommap[pos]) {
    // ...and not next to a room...
    if (!nexttoroom8(pos, validmap[pos])) {
      if (score) {
        // ...and it's freestanding, then give the best score
        return 1;
      } else if (CAN_CARVE(pos)) {
        // If the position is carvable, give a low score
        return 6;
      }
    }
  } else if (score) {
    // otherwise the position is in a room, so give a good score if it is
    // "more" freestanding (not in a corner).
    return score <= 9 ? 3 : 6;
  }
  return 0; // Invalid start position.
}

void fillends(void) {
  u8 pos, valid;
  memset(tempmap, 0, sizeof(tempmap));

  // Find all starting positions (dead ends)
  num_cands = 0;
  pos = 0;
  do {
    update_fill1(pos);
  } while(++pos);

  do {
    if (num_cands) {
      // Choose a random point, and calculate its offset
      pos = getpos(randint(num_cands));

      tmap[pos] = TILE_WALL; // Set tile to wall
      sigwall(pos);          // Update neighbor signatures.

      // Remove this cell from the dead end map
      --num_cands;
      tempmap[pos] = 0;

      valid = validmap[pos];
      if (valid & VALID_L) { update_fill1(POS_L(pos)); }
      if (valid & VALID_R) { update_fill1(POS_R(pos)); }
      if (valid & VALID_U) { update_fill1(POS_U(pos)); }
      if (valid & VALID_D) { update_fill1(POS_D(pos)); }
    }
  } while (num_cands);
}

void update_fill1(u8 pos) {
  u8 tile = tmap[pos];
  u8 result = !IS_WALL_TILE(tmap[pos]) && CAN_CARVE(pos) &&
              tile != TILE_START && tile != TILE_END;
  if (tempmap[pos] != result) {
    if (result) {
      ++num_cands;
    } else {
      --num_cands;
    }
    tempmap[pos] = result;
  }
}

void prettywalls(void) {
  u8 pos, tile;
  pos = 0;
  do {
    if (tmap[pos] == TILE_FIXED_WALL) {
      tmap[pos] = TILE_WALL;
    }
    if (tmap[pos] == TILE_WALL) {
      tile = sigmatch(pos, wallsig, wallmask);
      if (tile) {
        tile += TILE_WALLSIG_BASE;
      }
      if (TILE_HAS_CRACKED_VARIANT(tile) && XRND_12_5_PERCENT()) {
        tile += TILE_CRACKED_DIFF;
      }
      tmap[pos] = tile;
    } else if (tmap[pos] == TILE_EMPTY) {
      if (validmap[pos] & VALID_U) {
        if (IS_WALL_TILE(tmap[POS_U(pos)])) {
          tmap[pos] = IS_CRACKED_WALL_TILE(tmap[POS_U(pos)])
                          ? TILE_WALL_FACE_CRACKED
                          : TILE_WALL_FACE;
        }
      } else {
        tmap[pos] = TILE_WALL_FACE;
      }
    }
  } while(++pos);
}

void voids(void) {
  u8 spos, pos, head, oldtail, newtail, valid, newpos, num_walls, id, exit;

  spos = 0;
  num_voids = 0;

  memset(void_num_tiles, 0, sizeof(void_num_tiles));
  // Use the tempmap to keep track of void walls
  memset(tempmap, 0, sizeof(tempmap));

  do {
    if (!tmap[spos]) {
      // flood fill this region
      num_walls = 0;
      cands[head = 0] = spos;
      newtail = 1;
      id = num_rooms + num_voids + 1;

      do {
        oldtail = newtail;
        do {
          pos = cands[head++];
          if (!tmap[pos]) {
            roommap[pos] = id;
            ++void_num_tiles[num_voids];
            tmap[pos] = void_tiles[randint(sizeof(void_tiles))];
            valid = validmap[pos];

            if (valid & VALID_L) {
              if (!tmap[newpos = POS_L(pos)]) {
                cands[newtail++] = newpos;
              } else if (TILE_HAS_CRACKED_VARIANT(tmap[newpos])) {
                ++num_walls;
                tempmap[newpos] = id;
              }
            }
            if (valid & VALID_R) {
              if (!tmap[newpos = POS_R(pos)]) {
                cands[newtail++] = newpos;
              } else if (TILE_HAS_CRACKED_VARIANT(tmap[newpos])) {
                ++num_walls;
                tempmap[newpos] = id;
              }
            }
            if (valid & VALID_U) {
              if (!tmap[newpos = POS_U(pos)]) {
                cands[newtail++] = newpos;
              } else if (TILE_HAS_CRACKED_VARIANT(tmap[newpos])) {
                ++num_walls;
                tempmap[newpos] = id;
              }
            }
            if (valid & VALID_D) {
              if (!tmap[newpos = POS_D(pos)]) {
                cands[newtail++] = newpos;
              } else if (TILE_HAS_CRACKED_VARIANT(tmap[newpos])) {
                ++num_walls;
                tempmap[newpos] = id;
              }
            }
          }
        } while(head != oldtail);
      } while (oldtail != newtail);

      // Pick a random wall to make into a void exit
      exit = randint(num_walls);
      newpos = 0;
      do {
        if (tempmap[newpos] == id && exit-- == 0) { break; }
      } while(++newpos);

      void_exit[num_voids] = newpos;

      // Make sure the void exit is not a cracked wall.
      if (IS_CRACKED_WALL_TILE(tmap[newpos])) {
        tmap[newpos] -= TILE_CRACKED_DIFF;
      }
      ++num_voids;
    }
  } while(++spos);
}

void chests(void) {
  u8 has_teleporter, room, chest_pos, i, pos, cand0, cand1;

  // Teleporter in level if level >= 5 and rnd(5)<1 (20%)
  has_teleporter = floor >= 5 && XRND_20_PERCENT();

  // For all rooms, find a chest position (not on a wall).
  num_cands = 0;
  for (room = 0; room < num_rooms; ++room) {
    u8 maxw = room_w[room], maxh = room_h[room];
    pos = room_pos[room] + 17;
    for (u8 h = 1; h < maxh - 1; ++h, pos += 18 - maxw) {
      for (u8 w = 1; w < maxw - 1; ++w, ++pos) {
        cands[num_cands++] = pos;
      }
    }
  }

  chest_pos = cands[randint(num_cands)];

  // Pick a random position in each void and add it to the candidates. If
  // has_teleporter is true, then add chest_pos too.
  // The candidate indexes are stored starting at `num_voids`, so that the
  // actual positions can be stored starting at 0.
  for (i = 0; i < num_voids; ++i) {
    cands[1 + num_rooms + num_voids + i] = randint(void_num_tiles[i]);
  }

  // Convert these candidate to positions
  pos = 0;
  do {
    if (cands[num_voids + roommap[pos]]-- == 0) {
      // Subtract num_rooms so that the void regions starts at candidate 0 and
      // the rooms are not included.
      cands[(u8)(roommap[pos] - num_rooms - 1)] = pos;
    }
  } while (++pos);

  num_cands = num_voids;
  if (has_teleporter) {
    cands[num_cands++] = chest_pos;
  }

  if (num_cands > 2) {
    // Pick two random candidates and turn them into teleporters
    cand0 = randint(num_cands);
    cand1 = randint(num_cands - 1);
    if (cand1 == cand0) { ++cand1; }
    tmap[cands[cand0]] = tmap[cands[cand1]] = TILE_TELEPORTER;

    // Swap out the already used candidates
    cands[cand0] = cands[--num_cands];
    cands[cand1] = cands[--num_cands];
  }

  // Add chest position as a candidate if it wasn't already included above
  if (!has_teleporter) {
    cands[num_cands++] = chest_pos;
  }

  for (i = 0; i < num_cands; ++i) {
    addmob(floor >= 5 && XRND_20_PERCENT() ? MOB_TYPE_HEART_CHEST
                                            : MOB_TYPE_CHEST,
           cands[i]);
  }
}

void decoration(void) {
  RoomKind kind = ROOM_KIND_VASE;
  const u8 *room_perm = permutation_4 + (randint(24) << 2);
  u8 i, room, pos, tile, w, h, mob;
  for (i = 0; i < MAX_ROOMS; ++i) {
    room = *room_perm++;
    if (room >= num_rooms) { continue; }

    pos = room_pos[room];
    h = room_h[room];
    do {
      w = room_w[room];
      do {
        tile = tmap[pos];

        switch (kind) {
          case ROOM_KIND_VASE:
            mob = vase_mobs[randint(sizeof(vase_mobs))];
            if (mob && !IS_SMARTMOB_TILE(tile, pos) && IS_FREESTANDING(pos) &&
                (h == room_h[room] || h == 1 || w == room_w[room] || w == 1)) {
              addmob(mob, pos);
            }
            break;

          case ROOM_KIND_DIRT:
            if (tile == TILE_EMPTY) {
              tmap[pos] = dirt_tiles[xrnd() & 3];
            }
            break;

          case ROOM_KIND_CARPET:
            if (tile == TILE_EMPTY && h != room_h[room] && h != 1 &&
                w != room_w[room] && w != 1) {
              tmap[pos] = TILE_CARPET;
            }
            // fallthrough

          case ROOM_KIND_TORCH:
            if (tile == TILE_EMPTY && XRND_33_PERCENT() && (h & 1) &&
                IS_FREESTANDING(pos)) {
              if (w == 1) {
                tmap[pos] = TILE_TORCH_RIGHT;
              } else if (w == room_w[room]) {
                tmap[pos] = TILE_TORCH_LEFT;
              }
            }
            break;

          case ROOM_KIND_PLANT:
            if (tile == TILE_EMPTY) {
              tmap[pos] = plant_tiles[randint(3)];
            } else if (tile == TILE_WALL_FACE) {
              tmap[pos] = TILE_WALL_FACE_PLANT;
            }

            if (!IS_SMARTMOB_TILE(tile, pos) && room + 1 != start_room &&
                XRND_25_PERCENT() && no_mob_neighbors(pos) &&
                IS_FREESTANDING(pos)) {
              mob = plant_mobs[randint(sizeof(plant_mobs))];
              addmob(mob, pos);
              if (mob == MOB_TYPE_WEED) {
                room_avoid[room] = 1;
              }
            }
            break;
        }

        if (kind != ROOM_KIND_PLANT && room + 1 != start_room &&
            tmap[pos] == TILE_EMPTY && !mobmap[pos] && IS_FREESTANDING(pos) &&
            XRND_10_PERCENT()) {
          tmap[pos] = TILE_SAW;
          sigwall(pos);
        }

        ++pos;
      } while(--w);
      pos += MAP_WIDTH - room_w[room];
    } while(--h);


    // Pick a room kind for the next room
    kind = randint(NUM_ROOM_KINDS);
  }
}

void spawnmobs(void) {
  u8 pos, room, mob, mobs, i;
  u8 pack = 0;

  mobs = mob_plan_floor[floor + 1] - mob_plan_floor[floor];

retry:
  pos = 0;
  num_cands = 0;
  do {
    room = roommap[pos];
    if (room && room != start_room && room <= num_rooms &&
        !IS_SMARTMOB_TILE(tmap[pos], pos) &&
        (!room_avoid[room - 1] || (pack && no_mob_neighbors(pos)))) {
      tempmap[pos] = 1;
      ++num_cands;
    } else {
      tempmap[pos] = 0;
    }
  } while (++pos);

  do {
    pos = getpos(randint(num_cands));
    tempmap[pos] = 0;
    --num_cands;
    --mobs;
    addmob(mob_plan[mob_plan_floor[floor] + mobs], pos);
  } while (mobs && num_cands);

  if (mobs != 0 && !pack) {
    pack = 1;
    goto retry;
  }

  // Give a mob a key, if this is a gate floor
  if (dogate) {
    num_cands = 0;
    for (i = 0; i < num_mobs; ++i) {
      if (mob_type_key[mob_type[i]]) { ++num_cands; }
    }

    i = 0;
    mob = randint(num_cands);
    while (1) {
      if (mob_type_key[mob_type[i]] && mob-- == 0) { break; }
      ++i;
    }
    key_mob = i + 1;
  } else {
    key_mob = 0;
  }
}

void initdtmap(void) {
  u8 pos = 0;
  do {
    dtmap[pos] = fogmap[pos] ? 0 : tmap[pos];
  } while(++pos);
}

void initflagmap(void) {
  u8 pos = 0;
  do {
    flagmap[pos] = flags_bin[tmap[pos]];
  } while(++pos);
}

u8 no_mob_neighbors(u8 pos) {
  u8 valid = validmap[pos];
  return !(((valid & VALID_U) && mobmap[POS_U(pos)]) ||
           ((valid & VALID_L) && mobmap[POS_L(pos)]) ||
           ((valid & VALID_R) && mobmap[POS_R(pos)]) ||
           ((valid & VALID_D) && mobmap[POS_D(pos)]));
}

void mapset(u8* pos, u8 w, u8 h, u8 val) {
  u8 w2 = w;
  do {
    *pos++ = val;
    if (--w2 == 0) {
      w2 = w;
      --h;
      pos += MAP_WIDTH - w;
    }
  } while (h);
}

void sigrect_empty(u8 pos, u8 w, u8 h) {
  u8 w2 = w;
  do {
    sigempty(pos);
    ++pos;

    if (--w2 == 0) {
      w2 = w;
      --h;
      pos += MAP_WIDTH - w;
    }
  } while (h);
}

void sigempty(u8 pos) {
  u8 valid = validmap[pos];
  if (valid & VALID_UL) { sigmap[POS_UL(pos)] &= MASK_DR; }
  if (valid & VALID_U)  { sigmap[POS_U (pos)] &= MASK_D; }
  if (valid & VALID_UR) { sigmap[POS_UR(pos)] &= MASK_DL; }
  if (valid & VALID_L)  { sigmap[POS_L (pos)] &= MASK_R; }
  if (valid & VALID_R)  { sigmap[POS_R (pos)] &= MASK_L; }
  if (valid & VALID_DL) { sigmap[POS_DL(pos)] &= MASK_UR; }
  if (valid & VALID_D)  { sigmap[POS_D (pos)] &= MASK_U; }
  if (valid & VALID_DR) { sigmap[POS_DR(pos)] &= MASK_UL; }
}

void sigwall(u8 pos) {
  u8 valid = validmap[pos];
  if (valid & VALID_UL) { sigmap[POS_UL(pos)] |= VALID_DR; }
  if (valid & VALID_U)  { sigmap[POS_U (pos)] |= VALID_D; }
  if (valid & VALID_UR) { sigmap[POS_UR(pos)] |= VALID_DL; }
  if (valid & VALID_L)  { sigmap[POS_L (pos)] |= VALID_R; }
  if (valid & VALID_R)  { sigmap[POS_R (pos)] |= VALID_L; }
  if (valid & VALID_DL) { sigmap[POS_DL(pos)] |= VALID_UR; }
  if (valid & VALID_D)  { sigmap[POS_D (pos)] |= VALID_U; }
  if (valid & VALID_DR) { sigmap[POS_DR(pos)] |= VALID_UL; }
}

u8 sigmatch(u16 pos, const u8* sig, const u8* mask) {
  u8 result = 0;
  u8 val = sigmap[pos];
  while (1) {
    if (*sig == 0) { return 0; }
    if ((val | *mask) == *sig) {
      return result + 1;
    }
    ++mask;
    ++sig;
    ++result;
  }
}

u8 ds_union(u8 x, u8 y) {
  if (!y) return 0;
  if (!x) return 0;  // TODO: remove this since x is always called w/ valid id
  x = ds_find(x);
  y = ds_find(y);

  if (x == y) return 0;

  if (ds_sizes[x] < ds_sizes[y]) {
    u8 t = x;
    x = y;
    y = t;
  }

  ds_parents[y] = x;
  ds_sizes[x] = ds_sizes[x] + ds_sizes[y];
  --ds_num_sets;
  return 1;
}

u8 ds_find(u8 x) {
  // Use path-halving as described in
  // https://en.wikipedia.org/wiki/Disjoint-set_data_structure
  while (x != ds_parents[x]) {
    x = ds_parents[x] = ds_parents[ds_parents[x]];
  }
  return x;
}

u8 ds_find_pos(u8 pos) {
  return ds_find(ds_sets[pos]);
}

u8 getpos(u8 cand) {
  u8 pos = 0;
  do {
    if (tempmap[pos] && cand-- == 0) { break; }
  } while (++pos);
  return pos;
}

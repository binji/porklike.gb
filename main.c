#include <gb/gb.h>
#include <rand.h>
#include <string.h>

typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;

extern const u8 tiles_bg_2bpp[];
extern const u8 map_bin[];
extern const u8 flags_bin[];

#define IS_WALL(tile) (flags_bin[tile] & 1)
#define HAS_CRACKED_VARIANT(tile) (flags_bin[tile] & 0b01000000)
#define IS_CRACKED_WALL(tile) (flags_bin[tile] & 0b00001000)

#define IS_DOOR(pos) sigmatch((pos), doorsig, doormask)
#define CAN_CARVE(pos) sigmatch((pos), carvesig, carvemask)
#define IS_FREESTANDING(pos) sigmatch((pos), freesig, freemask)

#define VALID_UL 0b00000001
#define VALID_DL 0b00000010
#define VALID_DR 0b00000100
#define VALID_UR 0b00001000
#define VALID_D  0b00010000
#define VALID_U  0b00100000
#define VALID_R  0b01000000
#define VALID_L  0b10000000
#define VALID_L_OR_UL  0b10000001
#define VALID_U_OR_UL  0b00100001

#define MASK_UL  0b11111110
#define MASK_DL  0b11111101
#define MASK_DR  0b11111011
#define MASK_UR  0b11110111
#define MASK_D   0b11101111
#define MASK_U   0b11011111
#define MASK_R   0b10111111
#define MASK_L   0b01111111

#define DIR_UL(pos)   ((u8)(pos + 239))
#define DIR_U(pos)    ((u8)(pos + 240))
#define DIR_UR(pos)   ((u8)(pos + 241))
#define DIR_L(pos)    ((u8)(pos + 255))
#define DIR_R(pos)    ((u8)(pos + 1))
#define DIR_DL(pos)   ((u8)(pos + 15))
#define DIR_D(pos)    ((u8)(pos + 16))
#define DIR_DR(pos)   ((u8)(pos + 17))

#define TILE_EMPTY 1
#define TILE_WALL 2
#define TILE_WALL_FACE 3
#define TILE_WALL_FACE_PLANT 5
#define TILE_CARPET 7
#define TILE_END 13
#define TILE_START 14
#define TILE_GOAL 62
#define TILE_TORCH_LEFT 63
#define TILE_TORCH_RIGHT 65
#define TILE_HEART 67
#define TILE_FIXED_WALL 84
#define TILE_STEPS 161

const u8 dirpos[]   = {0xff, 1, 0xf0, 16};  // L R U D
const u8 dirvalid[] = {VALID_L,VALID_R,VALID_U,VALID_D};

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

// floor(35 / x) for x in [3,10]
const u8 other_dim[] = {11, 8, 7, 5, 5, 4, 3, 3};

// 54  0101 0100 UL
// d6  1101 0110 U
// 92  1001 0010 UR
// 7c  0111 1100 L
// ff  1111 1111 M
// b3  1011 0011 R
// 68  0110 1000 DL
// e9  1110 1001 D
// a1  1010 0001 DR
const u8 validmap[] = {
  0x54,0xd6,0xd6,0xd6,0xd6,0xd6,0xd6,0xd6,0xd6,0xd6,0xd6,0xd6,0xd6,0xd6,0xd6,0x92,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x68,0xe9,0xe9,0xe9,0xe9,0xe9,0xe9,0xe9,0xe9,0xe9,0xe9,0xe9,0xe9,0xe9,0xe9,0xa1,
};

const u8 void_tiles[] = {9, 9, 9, 9, 9, 9, 10};
const u8 dirt_tiles[] = {1, 73, 74, 75};
const u8 plant_tiles[] = {11, 12, 70};

const u8 room_permutations[][4] = {
    {1, 2, 3, 4}, {1, 2, 4, 3}, {1, 3, 2, 4}, {1, 3, 4, 2}, {1, 4, 2, 3},
    {1, 4, 3, 2}, {2, 1, 3, 4}, {2, 1, 4, 3}, {2, 3, 1, 4}, {2, 3, 4, 1},
    {2, 4, 1, 3}, {2, 4, 3, 1}, {3, 1, 2, 4}, {3, 1, 4, 2}, {3, 2, 1, 4},
    {3, 2, 4, 1}, {3, 4, 1, 2}, {3, 4, 2, 1}, {4, 1, 2, 3}, {4, 1, 3, 2},
    {4, 2, 1, 3}, {4, 2, 3, 1}, {4, 3, 1, 2}, {4, 3, 2, 1},
};

void mapgen(void);
void roomgen(void);
void mazeworms(void);
void update_carve1(u8 pos);
u8 nexttoroom4(u8 pos, u8 valid);
u8 nexttoroom8(u8 pos, u8 valid);
void digworm(u8 pos);
void carvedoors(void);
void update_door1(u8 pos);
void carvecuts(void);
void calcdist(u8 pos);
void startend(void);
u8 startscore(u8 pos);
void fillends(void);
void update_fill1(u8 pos);
void prettywalls(void);
void voids(void);
void decoration(void);
void mapset(u8* pos, u8 w, u8 h, u8 val);
void sigrect_empty(u8 pos, u8 w, u8 h);
void sigempty(u8 pos);
void sigwall(u8 pos);
u8 sigmatch(u16 pos, u8* sig, u8* mask);
u8 ds_union(u8 x, u8 y);
u8 ds_find(u8 id);
u8 getpos(u8 cand);
u8 randint(u8 mx);

typedef u8 Map[16*16];

typedef enum RoomKind {
  ROOM_KIND_VASE,
  ROOM_KIND_DIRT,
  ROOM_KIND_CARPET,
  ROOM_KIND_TORCH,
  ROOM_KIND_PLANT,
  NUM_ROOM_KINDS,
} RoomKind;

Map tmap;    // Tile map
Map roommap; // Room map
Map sigmap;  // Signature map (tracks neighbors)
Map tempmap; // Temp map (used for carving)
Map distmap; // Distance map

// The following data structures is used for rooms as well as union-find
// algorithm; see https://en.wikipedia.org/wiki/Disjoint-set_data_structure
// XXX can be much smaller
Map ds_sets; // TODO seems like it should be possible to put this in roommap
u8 ds_parents[64];
u8 ds_sizes[64];
u8 ds_nextid;
u8 ds_num_sets;

u8 room_pos[4];
u8 room_w[4];
u8 room_h[4];

u8 start_room;
u8 num_rooms;
u8 num_voids;
u8 floor;
u8 dogate;
u8 startpos, endpos, goalpos;

u8 cands[256];
u8 num_cands;

u8 key;

u16 myclock;
void tim_interrupt(void) { ++myclock; }

void main(void) {
  DISPLAY_OFF;
  add_TIM(tim_interrupt);
  TMA_REG = 0;      // Divide clock by 256 => 16Hz
  TAC_REG = 0b100;  // 4096Hz, timer on.
  IE_REG |= TIM_IFLAG;

  enable_interrupts();

  floor = 0;
  initrand(0x4321);  // TODO: seed with DIV on button press

  set_bkg_data(0, 0xb1, tiles_bg_2bpp);
  init_bkg(0);
  mapgen();

  set_bkg_tiles(0, 0, 16, 16, tmap);

  LCDC_REG = 0b10000001;  // display on, bg on

  while(1) {
    key = joypad();

    // XXX
    if (key & J_A) {
      ++floor;
      if (floor == 11) { floor = 0; }
      DISPLAY_OFF;
      mapgen();
      set_bkg_tiles(0, 0, 16, 16, tmap);
      LCDC_REG = 0b10000001;  // display on, bg on
    }

    wait_vbl_done();
  }
}

u8 dummy[11];
#define TIMESTART dummy[0] = (u8)(myclock)
#define TIMEDIFF(x)                                                            \
  dummy[x] = (u8)(myclock)-dummy[0];                                           \
  TIMESTART

void mapgen(void) {
  if (floor == 0) {
    memcpy(tmap, map_bin, 256);
  } else if (floor == 10) {
    memcpy(tmap, map_bin + 256, 256);
  } else {
    TIMESTART;
    roomgen();
    TIMEDIFF(1);
    mazeworms();
    TIMEDIFF(2);
    carvedoors();
    TIMEDIFF(3);
    carvecuts();
    TIMEDIFF(4);
    startend();
    TIMEDIFF(5);
    fillends();
    TIMEDIFF(6);
    prettywalls();
    TIMEDIFF(7);
    voids();
    TIMEDIFF(8);
    decoration();
    TIMEDIFF(9);

    dummy[10] = dummy[1] + dummy[2] + dummy[3] + dummy[4] + dummy[5] +
                dummy[6] + dummy[7] + dummy[8] + dummy[9];
  }
}

void roomgen(void) {
  u8 failmax = 5, maxwidth = 10, maxheight = 10, maxh;
  u8 sig, valid, walkable, val, w, h, pos;

  dogate = floor == 3 || floor == 6 || floor == 9;

  memset(roommap, 0, sizeof(roommap));
  memset(sigmap, 255, sizeof(sigmap));
  memset(ds_sets, 0, sizeof(ds_sets));

  // Initialize disjoint set
  ds_nextid = 0;
  ds_parents[0] = 0;
  ds_num_sets = 0;

  if (dogate) {
    // Pick a random gate map
    memcpy(tmap, map_bin + 512 + (randint(14) << 8), 256);

    // Update signature map for empty tiles
    pos = 0;
    do {
      switch (tmap[pos]) {
        case TILE_END:
          endpos = pos;
          goto empty;

        case TILE_START:
          startpos = pos;
          goto empty;

        case TILE_GOAL:
          // Temporarily change to stairs so the empty spaces behind the gate
          // can be part of the spanning graph created during the carvedoors()
          // step.
          tmap[pos] = TILE_END;
          goalpos = pos;
          goto empty;

        case TILE_HEART:
          // TODO: add heart mob
          goto empty;

        empty:
        case TILE_CARPET:
        case TILE_EMPTY:
        case TILE_STEPS:
          // Update sigmap
          sigempty(pos);

          // Update disjoint set for the gate area
          ++ds_nextid;
          ds_sets[pos] = ds_nextid;
          ds_parents[ds_nextid] = ds_nextid;
          ds_sizes[ds_nextid] = 1;

          valid = validmap[pos];
          if (valid & VALID_U) { ds_union(ds_nextid, ds_sets[DIR_U(pos)]); }
          if (valid & VALID_L) { ds_union(ds_nextid, ds_sets[DIR_L(pos)]); }
          if (valid & VALID_R) { ds_union(ds_nextid, ds_sets[DIR_R(pos)]); }
          if (valid & VALID_D) { ds_union(ds_nextid, ds_sets[DIR_D(pos)]); }
          break;
      }

    } while (++pos);
  } else {
    // Start with a completely empty map (all walls)
    memset(tmap, TILE_WALL, sizeof(tmap));
  }

  num_rooms = 0;

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
      walkable = !IS_WALL(tmap[pos]);
      if (walkable || (sig & VALID_L_OR_UL)) {
        // This tile is either in a room, or on the right or lower border of
        // the room.
        val = h + 1;
      } else if (valid & VALID_D) {
        // This tile is above a room or the bottom edge; we subtract one from
        // the value below to track whether it is far enough away to fit a room
        // of height `h`.
        val = tempmap[DIR_D(pos)];
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
        val = distmap[DIR_R(pos)];
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
          tempmap[pos] = tempmap[DIR_R(pos)];
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

      // Fill room tiles at x, y
      mapset(tmap + pos, w, h, 1);
      mapset(roommap + pos, w, h, num_rooms);
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
  } while (failmax && num_rooms < 4);
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
      if (valid & VALID_U) { ds_union(ds_nextid, ds_sets[DIR_U(pos)]); }
      if (valid & VALID_L) { ds_union(ds_nextid, ds_sets[DIR_L(pos)]); }
      if (valid & VALID_R) { ds_union(ds_nextid, ds_sets[DIR_R(pos)]); }
      if (valid & VALID_D) { ds_union(ds_nextid, ds_sets[DIR_D(pos)]); }
    }
  } while(num_cands > 1);
}

void update_carve1(u8 pos) {
  u8 tile = tmap[pos];
  u8 result;
  if (tile != TILE_FIXED_WALL && IS_WALL(tile) && CAN_CARVE(pos)) {
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
  return ((valid & VALID_U) && roommap[DIR_U(pos)]) ||
         ((valid & VALID_L) && roommap[DIR_L(pos)]) ||
         ((valid & VALID_R) && roommap[DIR_R(pos)]) ||
         ((valid & VALID_D) && roommap[DIR_D(pos)]);
}

u8 nexttoroom8(u8 pos, u8 valid) {
  return nexttoroom4(pos, valid) ||
         ((valid & VALID_UL) && roommap[DIR_UL(pos)]) ||
         ((valid & VALID_UR) && roommap[DIR_UR(pos)]) ||
         ((valid & VALID_DL) && roommap[DIR_DL(pos)]) ||
         ((valid & VALID_DR) && roommap[DIR_DR(pos)]);
}

void digworm(u8 pos) {
  u8 dir, step, num_dirs, valid;

  // Pick a random direction
  dir = rand() & 3;
  step = 0;
  do {
    tmap[pos] = 1; // Set tile to empty.
    sigempty(pos); // Update neighbor signatures.

    // Remove this tile from the carvable map
    if (tempmap[pos] == 2) { --num_cands; }
    tempmap[pos] = 0;

    // Update neighbors
    valid = validmap[pos];
    if (valid & VALID_UL) { update_carve1(DIR_UL(pos)); }
    if (valid & VALID_U)  { update_carve1(DIR_U (pos)); }
    if (valid & VALID_UR) { update_carve1(DIR_UR(pos)); }
    if (valid & VALID_L)  { update_carve1(DIR_L (pos)); }
    if (valid & VALID_R)  { update_carve1(DIR_R (pos)); }
    if (valid & VALID_DL) { update_carve1(DIR_DL(pos)); }
    if (valid & VALID_D)  { update_carve1(DIR_D (pos)); }
    if (valid & VALID_DR) { update_carve1(DIR_DR(pos)); }

    // Update the disjoint set
    ds_sets[pos] = ds_nextid;
    ++ds_sizes[ds_nextid];

    // Continue in the current direction, unless we can't carve. Also randomly
    // change direction after 3 or more steps.
    if (!((valid & dirvalid[dir]) && tempmap[(u8)(pos + dirpos[dir])]) ||
        ((rand() & 1) && step > 2)) {
      step = 0;
      num_dirs = 0;
      if ((valid & VALID_L) && tempmap[DIR_L(pos)]) { cands[num_dirs++] = 0; }
      if ((valid & VALID_R) && tempmap[DIR_R(pos)]) { cands[num_dirs++] = 1; }
      if ((valid & VALID_U) && tempmap[DIR_U(pos)]) { cands[num_dirs++] = 2; }
      if ((valid & VALID_D) && tempmap[DIR_D(pos)]) { cands[num_dirs++] = 3; }
      if (num_dirs == 0) return;
      dir = cands[randint(num_dirs)];
    }
    pos += dirpos[dir];
    ++step;
  } while(1);
}

void carvedoors(void) {
  u8 pos, match, diff;

  memset(tempmap, 0, sizeof(tempmap));

  pos = 0;
  num_cands = 0;
  do {
    update_door1(pos);
  } while(++pos);

  do {
    // Pick a random door, and calculate its position.
    pos = getpos(randint(num_cands));;

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
      if (valid & VALID_U) { update_door1(DIR_U(pos)); }
      if (valid & VALID_L) { update_door1(DIR_L(pos)); }
      if (valid & VALID_R) { update_door1(DIR_R(pos)); }
      if (valid & VALID_D) { update_door1(DIR_D(pos)); }
    }

    // Remove this tile from the carvable map
    --num_cands;
    tempmap[pos] = 0;
  } while (num_cands);

  // TODO: its possible to generate a map that is not fully-connected; in that
  // case we need to start over.
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
  u8 result = tile != TILE_FIXED_WALL && IS_WALL(tile) && IS_DOOR(pos);
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
    // TODO: we only need the distance to the other size of the door; use a
    // faster routine for this?
    calcdist(pos + diff);

    // Remove this candidate
    tempmap[pos] = 0;
    --num_cands;

    // Connect the regions (creating a cycle) if the distance between the two
    // sides is > 20 steps (21 is used because the distance starts at 1; that's
    // so that 0 can be used to represent an unvisited cell).
    if (distmap[(u8)(pos - diff)] > 21) {
      tmap[pos] = 1;
      sigempty(pos); // Update neighbor signatures.
      --maxcuts;
    }
  } while (num_cands && maxcuts);
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
      if (!distmap[newpos = DIR_U(pos)]) {
        if (valid & VALID_U) { distmap[newpos] = dist; }
        if (sig & VALID_U) { cands[newtail++] = newpos; }
      }
      if (!distmap[newpos = DIR_L(pos)]) {
        if (valid & VALID_L) { distmap[newpos] = dist; }
        if (sig & VALID_L) { cands[newtail++] = newpos; }
      }
      if (!distmap[newpos = DIR_R(pos)]) {
        if (valid & VALID_R) { distmap[newpos] = dist; }
        if (sig & VALID_R) { cands[newtail++] = newpos; }
      }
      if (!distmap[newpos = DIR_D(pos)]) {
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
      endpos = rand();
    } while (IS_WALL(tmap[endpos]));
  }

  calcdist(endpos);

  startpos = 0;
  best = 0;
  // Find the furthest point from endpos.
  pos = 0;
  do {
    if (distmap[pos] > best) {
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

  tmap[startpos] = TILE_START;
  tmap[endpos] = TILE_END;
  sigempty(startpos);
  sigempty(endpos);

  if (dogate) {
    tmap[goalpos] = TILE_GOAL;
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

      tmap[pos] = 2; // Set tile to wall
      sigwall(pos);  // Update neighbor signatures.

      // Remove this cell from the dead end map
      --num_cands;
      tempmap[pos] = 0;

      valid = validmap[pos];
      if (valid & VALID_UL) { update_fill1(DIR_UL(pos)); }
      if (valid & VALID_U)  { update_fill1(DIR_U (pos)); }
      if (valid & VALID_UR) { update_fill1(DIR_UR(pos)); }
      if (valid & VALID_L)  { update_fill1(DIR_L (pos)); }
      if (valid & VALID_R)  { update_fill1(DIR_R (pos)); }
      if (valid & VALID_DL) { update_fill1(DIR_DL(pos)); }
      if (valid & VALID_D)  { update_fill1(DIR_D (pos)); }
      if (valid & VALID_DR) { update_fill1(DIR_DR(pos)); }
    }
  } while (num_cands);
}

void update_fill1(u8 pos) {
  u8 tile = tmap[pos];
  u8 result = !IS_WALL(tmap[pos]) && CAN_CARVE(pos) && tile != TILE_START &&
              tile != TILE_END;
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
        tile += 14;
      }
      if (HAS_CRACKED_VARIANT(tile) && !(rand() & 7)) {
        // TODO: organize tiles so this can be a simple addition
        switch (tile) {
          case 0x10: tile = 0x9d; break;
          case 0x1f: tile = 0xab; break;
          case 0x21: tile = 0xac; break;
          case 0x30: tile = 0xb0; break;
          case 0x32: tile = 0x9e; break;
          case 0x33: tile = 0x9f; break;
        }
      }
      tmap[pos] = tile;
    } else if (tmap[pos] == TILE_EMPTY && (validmap[pos] & VALID_U) &&
               IS_WALL(tmap[DIR_U(pos)])) {
      tmap[pos] = IS_CRACKED_WALL(tmap[DIR_U(pos)]) ? 0xa0 : 3;
    }
  } while(++pos);
}

void voids(void) {
  u8 pos, head, oldtail, newtail, valid, newpos;
  pos = 0;
  num_voids = num_rooms;  // Start void numbering after rooms

  do {
    if (!tmap[pos]) {
      // flood fill this region
      cands[head = 0] = pos;
      newtail = 1;
      do {
        oldtail = newtail;
        do {
          pos = cands[head++];
          roommap[pos] = num_voids;
          tmap[pos] = void_tiles[randint(sizeof(void_tiles))];
          valid = validmap[pos];

          // TODO: handle void exits
          if ((valid & VALID_U) && !tmap[newpos = DIR_U(pos)]) {
            cands[newtail++] = newpos;
          }
          if ((valid & VALID_L) && !tmap[newpos = DIR_L(pos)]) {
            cands[newtail++] = newpos;
          }
          if ((valid & VALID_R) && !tmap[newpos = DIR_R(pos)]) {
            cands[newtail++] = newpos;
          }
          if ((valid & VALID_D) && !tmap[newpos = DIR_D(pos)]) {
            cands[newtail++] = newpos;
          }
        } while(head != oldtail);
      } while (oldtail != newtail);
      ++num_voids;
    }
  } while(++pos);
}

void decoration(void) {
  RoomKind kind = ROOM_KIND_VASE;
  const u8 (*room_perm)[4] = room_permutations + randint(24);
  u8 i, room, pos, tile, w, h;
  for (i = 0; i < 4; ++i) {
    room = (*room_perm)[i];
    if (room >= num_rooms) { continue; }

    pos = room_pos[room];
    h = room_h[room];
    do {
      w = room_w[room];
      do {
        tile = tmap[pos];

        switch (kind) {
          case ROOM_KIND_VASE:
            break; // TODO: need mobs

          case ROOM_KIND_DIRT:
            if (tile == TILE_EMPTY) {
              tmap[pos] = dirt_tiles[rand() & 3];
            }
            break;

          case ROOM_KIND_CARPET:
            if (tile == TILE_EMPTY && h != room_h[room] && h != 1 &&
                w != room_w[room] && w != 1) {
              tmap[pos] = TILE_CARPET;
            }
            // fallthrough

          case ROOM_KIND_TORCH:
            if (tile == TILE_EMPTY && (rand() < 85) && (h & 1) &&
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
            // TODO: add weed/bomb mobs
            break;
        }

        ++pos;
      } while(--w);
      pos += 16 - room_w[room];
    } while(--h);


    // Pick a room kind for the next room
    kind = randint(NUM_ROOM_KINDS);
  }
}

void mapset(u8* pos, u8 w, u8 h, u8 val) {
  u8 w2 = w;
  do {
    *pos++ = val;
    if (--w2 == 0) {
      w2 = w;
      --h;
      pos += 16 - w;
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
      pos += 16 - w;
    }
  } while (h);
}

void sigempty(u8 pos) {
  u8 valid = validmap[pos];
  if (valid & VALID_UL) { sigmap[DIR_UL(pos)] &= MASK_DR; }
  if (valid & VALID_U)  { sigmap[DIR_U (pos)] &= MASK_D; }
  if (valid & VALID_UR) { sigmap[DIR_UR(pos)] &= MASK_DL; }
  if (valid & VALID_L)  { sigmap[DIR_L (pos)] &= MASK_R; }
  if (valid & VALID_R)  { sigmap[DIR_R (pos)] &= MASK_L; }
  if (valid & VALID_DL) { sigmap[DIR_DL(pos)] &= MASK_UR; }
  if (valid & VALID_D)  { sigmap[DIR_D (pos)] &= MASK_U; }
  if (valid & VALID_DR) { sigmap[DIR_DR(pos)] &= MASK_UL; }
}

void sigwall(u8 pos) {
  u8 valid = validmap[pos];
  if (valid & VALID_UL) { sigmap[DIR_UL(pos)] |= VALID_DR; }
  if (valid & VALID_U)  { sigmap[DIR_U (pos)] |= VALID_D; }
  if (valid & VALID_UR) { sigmap[DIR_UR(pos)] |= VALID_DL; }
  if (valid & VALID_L)  { sigmap[DIR_L (pos)] |= VALID_R; }
  if (valid & VALID_R)  { sigmap[DIR_R (pos)] |= VALID_L; }
  if (valid & VALID_DL) { sigmap[DIR_DL(pos)] |= VALID_UR; }
  if (valid & VALID_D)  { sigmap[DIR_D (pos)] |= VALID_U; }
  if (valid & VALID_DR) { sigmap[DIR_DR(pos)] |= VALID_UL; }
}

u8 sigmatch(u16 pos, u8* sig, u8* mask) {
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

u8 getpos(u8 cand) {
  u8 pos = 0;
  do {
    if (tempmap[pos] && cand-- == 0) { break; }
  } while (++pos);
  return pos;
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

u8 randint(u8 mx) {
  if (mx == 0) { return 0; }
  u8 result;
  do {
    result = rand() & randmask[mx];
  } while (result >= mx);
  return result;
}

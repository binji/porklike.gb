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

#define VALID_UL 0b00000001
#define VALID_DL 0b00000010
#define VALID_DR 0b00000100
#define VALID_UR 0b00001000
#define VALID_D  0b00010000
#define VALID_U  0b00100000
#define VALID_R  0b01000000
#define VALID_L  0b10000000

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

const u8 dirpos[]   = {0xff, 1, 0xf0, 16};  // L R U D
const u8 dirvalid[] = {VALID_L,VALID_R,VALID_U,VALID_D};

const u8 carvesig[] = {255, 214, 124, 179, 233};
const u8 carvemask[] = {0, 9, 3, 12, 6};
const u8 doorsig[] = {192, 48};
const u8 doormask[] = {15, 15};

const u8 freesig[] = {0, 0, 0, 0, 16, 64, 32, 128, 161, 104, 84, 146};
const u8 freemask[] = {8, 4, 2, 1, 6, 12, 9, 3, 10, 5, 10, 5};

const u8 wallsig[] = {
    251, 233, 253, 84,  146, 80,  16,  144, 112, 208, 241, 248,
    210, 177, 225, 120, 179, 0,   124, 104, 161, 64,  240, 128,
    224, 176, 242, 244, 116, 232, 178, 212, 247, 214, 254, 192,
    48,  96,  32,  160, 245, 250, 243, 249, 246, 252,
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
void append_region(u8 x, u8 y, u8 w, u8 h);
void mapset(u8* pos, u8 w, u8 h, u8 val);
void sigrect_empty(u8 pos, u8 w, u8 h);
void sigempty(u8 pos);
void sigwall(u8 pos);
u8 sigmatch(u16 pos, u8* sig, u8* mask, u8 len);
u8 ds_union(u8 x, u8 y);
u8 ds_find(u8 id);
u8 getpos(u8 cand);
u8 randint(u8 mx);

typedef u8 Map[16*16];

typedef struct Region {
  u8 x, y, w, h;
} Region;

typedef enum RoomKind {
  ROOM_KIND_VASE,
  ROOM_KIND_DIRT,
  ROOM_KIND_CARPET,
  ROOM_KIND_TORCH,
  ROOM_KIND_PLANT,
  NUM_ROOM_KINDS,
} RoomKind;

typedef struct Room {
  u8 x, y, w, h;
} Room;

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

 // TODO: I think 22 is max for 4 rooms...
Region regions[22], *region_end, *new_region_end;
Room rooms[4];
u8 start_room;
u8 num_rooms;
u8 num_voids;

u8 cands[256];
u8 num_cands;
u8 key;

void main(void) {
  disable_interrupts();
  DISPLAY_OFF;

  initrand(0x4321);  // TODO: seed with DIV on button press

  set_bkg_data(0, 0xb1, tiles_bg_2bpp);
  init_bkg(0);
  mapgen();

  set_bkg_tiles(0, 0, 16, 16, tmap);

  LCDC_REG = 0b10000001;  // display on, bg on
  enable_interrupts();

  while(1) {
    key = joypad();

    // XXX
    if (key & J_A) {
      disable_interrupts();
      DISPLAY_OFF;
      mapgen();
      set_bkg_tiles(0, 0, 16, 16, tmap);
      LCDC_REG = 0b10000001;  // display on, bg on
      enable_interrupts();
    }

    wait_vbl_done();
  }
}

void mapgen(void) {
  roomgen();
  mazeworms();
  carvedoors();
  carvecuts();
  startend();
  fillends();
  prettywalls();
  voids();
  decoration();
}

void roomgen(void) {
  Region *region, *old_region_end;
  Room* room;
  u8 failmax = 5, maxwidth = 10, maxheight = 10, maxh;
  u8 x, y, w, h;
  u8 regl, regr, regt, regb, regw, regh;
  u8 rooml, roomr, roomt, roomb;
  u16 pos;

  memset(tmap, 2, sizeof(tmap));
  memset(roommap, 0, sizeof(roommap));
  memset(sigmap, 255, sizeof(sigmap));
  memset(ds_sets, 0, sizeof(ds_sets));

  ds_nextid = 0;
  ds_parents[0] = 0;
  ds_num_sets = 0;
  num_rooms = 0;

  regions[0].x = regions[0].y = 1; regions[0].w = regions[0].h = 16;
  region_end = regions + 1;

  do {
    // Pick width and height for room.
    w = 3 + randint(maxwidth - 2);
    maxh = other_dim[w - 3];
    if (maxh > maxheight) maxh = maxheight;
    h = 3 + randint(maxh - 2);

    // Try to place a room
    // Find valid regions
    num_cands = 0;
    region = regions;
    while (region < region_end) {
      if (w <= region->w && h <= region->h) {
        cands[num_cands++] = region - regions;
      }
      ++region;
    }

    if (num_cands) {
      // TODO: this method is biased toward regions with overlap; probably want
      // to pick a random location first, then determine if it is valid (i.e.
      // in a region).

      // Pick a random region
      region = regions + cands[randint(num_cands)];

      // Pick a random position in the region
      x = region->x + randint(region->w - w);
      y = region->y + randint(region->h - h);

      old_region_end = new_region_end = region_end;
      region = regions;
      do {
        regl = region->x;
        regt = region->y;
        regw = region->w;
        regh = region->h;
        regr = regl + regw;
        regb = regt + regh;

        rooml = x - 1;
        roomr = x + w + 1;
        roomt = y - 1;
        roomb = y + h + 1;

        // Only split this region if it contains the room (include 1 tile
        // border)
        if (roomr > regl && roomb > regt && rooml < regr && roomt < regb) {
          // We want to split this region, which means it is replaced by up to
          // 4 other regions. If this is not the last region to process, then
          // we copy that region on top of this one to pack the regions. This
          // means that we have to process this region again (since it
          // changed).
          if (--old_region_end != region) {
            *region = *old_region_end;
          } else {
            // However, if this is the last region, then we are done (so
            // increment region).
            ++region;
          }

          // Divide chosen region into new regions
          // Add top region
          if (roomt > regt) {
            append_region(regl, regt, regw, roomt - regt);
          }

          // Add bottom region
          if (roomb < regb) {
            append_region(regl, roomb, regw, regb - roomb);
          }

          // Add left region
          if (rooml > regl) {
            append_region(regl, regt, rooml - regl, regh);
          }

          // Add right region
          if (roomr < regr) {
            append_region(roomr, regt, regr - roomr, regh);
          }
        } else {
          ++region;
        }
      } while (region < old_region_end);

      // Fill over any regions that were removed between old_region_end and
      // region_end. e.g.
      // If the regions are:
      //   0 1 2 3
      //
      // and region 1 is split into 4 5 6, then the list is:
      //   0 3 2 x 4 5 6
      //
      // If region 2 is now split into 7 and 8:
      //   0 3 x x 4 5 6 7 8
      //
      // Now the new regions must be packed into the x's
      //   0 3 8 7 4 5 6
      while (old_region_end < region_end) {
        *old_region_end++ = *--new_region_end;
      }
      region_end = new_region_end;

      // Fill room tiles at x, y
      --x;
      --y;
      pos = (y << 4) | x;
      mapset(tmap + pos, w, h, 1);
      mapset(roommap + pos, w, h, num_rooms);
      sigrect_empty(pos, w, h);

      // Update disjoint set regions; we know that they're disjoint so they
      // each get their own region id (i.e. the room number)
      mapset(ds_sets + pos, w, h, ++ds_nextid);
      ds_parents[ds_nextid] = ds_nextid;
      ds_sizes[ds_nextid] = w * h; // XXX multiply
      ++ds_num_sets;

      room = rooms + num_rooms++;
      room->x = x;
      room->y = y;
      room->w = w;
      room->h = h;

      if (num_rooms == 1) {
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
  u8 result;
  if ((flags_bin[tmap[pos]] & 1) &&
      sigmatch(pos, carvesig, carvemask, sizeof(carvesig))) {
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
    match = sigmatch(pos, doorsig, doormask, sizeof(doorsig));
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
  u8 result = (flags_bin[tmap[pos]] & 1) &&
              sigmatch(pos, doorsig, doormask, sizeof(doorsig));
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
    match = sigmatch(pos, doorsig, doormask, sizeof(doorsig));
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
  u8 pos, startpos, endpos, best, score;

  // Pick a random walkable location
  do {
    endpos = rand();
  } while (flags_bin[tmap[endpos]] & 1);

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

  // Now pick the furthest, freestanding location in a room from the startpos.
  pos = 0;
  best = 0;
  do {
    if (distmap[pos] > best && roommap[pos] &&
        sigmatch(pos, freesig, freemask, sizeof(freesig))) {
      endpos = pos;
      best = distmap[pos];
    }
  } while(++pos);

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

  tmap[startpos] = 14;
  tmap[endpos] = 13;
  sigempty(startpos);
  sigempty(endpos);
}

u8 startscore(u8 pos) {
  u8 score = sigmatch(pos, freesig, freemask, sizeof(freesig));
  // If the position is not in a room...
  if (!roommap[pos]) {
    // ...and not next to a room...
    if (!nexttoroom8(pos, validmap[pos])) {
      if (score) {
        // ...and it's freestanding, then give the best score
        return 1;
      } else if (sigmatch(pos, carvesig, carvemask, sizeof(carvesig))) {
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
  u8 result = !(flags_bin[tmap[pos]] & 1) &&
              sigmatch(pos, carvesig, carvemask, sizeof(carvesig)) &&
              tile != 13 && tile != 14;
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
    if (tmap[pos] == 2) {
      tile = sigmatch(pos, wallsig, wallmask, sizeof(wallsig));
      if (tile) {
        tile += 14;
      }
      if ((flags_bin[tile] & 0b01000000) && !(rand() & 7)) {
        // TODO: organize tiles so this can be a simple addition
        switch (tile) {
          case 0x10: tile = 0x9c; break;
          case 0x1f: tile = 0xaa; break;
          case 0x21: tile = 0xab; break;
          case 0x30: tile = 0xaf; break;
          case 0x32: tile = 0x9d; break;
          case 0x33: tile = 0x9e; break;
        }
      }
      tmap[pos] = tile;
    } else if (tmap[pos] == 1 && (validmap[pos] & VALID_U) &&
               (flags_bin[tmap[DIR_U(pos)]] & 1)) {
      tmap[pos] = (flags_bin[tmap[DIR_U(pos)]] & 0b00001000) ? 0x9f : 3;
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
  Room* room;
  const u8 (*room_perm)[4] = room_permutations + randint(24);
  u8 i, room_index, pos, tile, w, h;
  for (i = 0; i < 4; ++i) {
    room_index = (*room_perm)[i];
    if (room_index >= num_rooms) { continue; }

    room = rooms + room_index;
    pos = (room->y << 4) | room->x;
    h = room->h;
    do {
      w = room->w;
      do {
        tile = tmap[pos];

        switch (kind) {
          case ROOM_KIND_VASE:
            break; // TODO: need mobs

          case ROOM_KIND_DIRT:
            if (tile == 1) {
              tmap[pos] = dirt_tiles[rand() & 3];
            }
            break;

          case ROOM_KIND_CARPET:
            if (tile == 1 && h != room->h && h != 1 && w != room->w &&
                w != 1) {
              tmap[pos] = 7;
            }
            // fallthrough

          case ROOM_KIND_TORCH:
            if (tile == 1 && (rand() < 85) && (h & 1) &&
                sigmatch(pos, freesig, freemask, sizeof(freesig))) {
              if (w == 1) {
                tmap[pos] = 65;
              } else if (w == room->w) {
                tmap[pos] = 63;
              }
            }
            break;

          case ROOM_KIND_PLANT:
            if (tile == 1) {
              tmap[pos] = plant_tiles[randint(3)];
            } else if (tile == 3) {
              tmap[pos] = 5;
            }
            // TODO: add weed/bomb mobs
            break;
        }

        ++pos;
      } while(--w);
      pos += 16 - room->w;
    } while(--h);


    // Pick a room kind for the next room
    kind = randint(NUM_ROOM_KINDS);
  }
}

void append_region(u8 x, u8 y, u8 w, u8 h) {
  new_region_end->x = x;
  new_region_end->y = y;
  new_region_end->w = w;
  new_region_end->h = h;
  ++new_region_end;
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

u8 sigmatch(u16 pos, u8* sig, u8* mask, u8 len) {
  u8 result = 0;
  u8 val = sigmap[pos];
  do {
    if ((val | *mask) == (*sig | *mask)) {
      return result + 1;
    }
    ++mask;
    ++sig;
    ++result;
  } while(result < len);
  return 0;
}

u8 ds_union(u8 x, u8 y) {
  x = ds_find(x);
  if (!x) return 0;  // TODO: remove this since x is always called w/ valid id

  y = ds_find(y);
  if (!y) return 0;

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

u8 randint(u8 mx) {
  if (mx == 0) { return 0; }
  u8 mask = 1, mx2 = mx;
  do {
    mask <<= 1;
    mask |= 1;
    mx2 >>= 1;
  } while (mx2);

  u8 result;
  do {
    result = rand() & mask;
  } while (result >= mx);
  return result;
}

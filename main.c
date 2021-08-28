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

const u16 dirpos[] = {0xffff, 1, 0xffee, 18};

const u8 carvesig[] = {255, 214, 124, 179, 233};
const u8 carvemask[] = {0, 9, 3, 12, 6};
const u8 doorsig[] = {192, 48};
const u8 doormask[] = {15, 15};

// floor(35 / x) for x in [3,10]
const u8 other_dim[] = {11, 8, 7, 5, 5, 4, 3, 3};

void mapgen(void);
void roomgen(void);
void mazeworms(void);
void digworm(u16 pos);
u8 nexttoroom(u16 pos);
void update_carve1(u16 pos);
void carvedoors(void);
void update_door1(u16 pos);
void door9(u16 pos);
void carvecuts(void);
void append_region(u8 x, u8 y, u8 w, u8 h);
void blankmap(u8* map, u8 border, u8 def);
void mapset(u8* pos, u8 w, u8 h, u8 val);
void sigrect_empty(u8* pos, u8 w, u8 h);
void sigwall(u8* pos);
void sigempty(u8* pos);
u8 sigmatch(u16 pos, u8* sig, u8* mask, u8 len);
u16 getpos(u8 x, u8 y);
u8 ds_union(u8 x, u8 y);
u8 ds_find(u8 id);
u8 randint(u8 mx);

typedef u8 Map[18*18];

typedef struct Region {
  u8 x, y, w, h;
} Region;

typedef struct Room {
  u8 x, y, w, h;
} Room;

u8 key;
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
Room room[4];
u8 num_rooms;

u8 cands[256];
u8 num_cands;

void main(void) {
  disable_interrupts();
  DISPLAY_OFF;

  initrand(0x1234);  // TODO: seed with DIV on button press

  set_bkg_data(0, 0xb1, tiles_bg_2bpp);
  init_bkg(0);
  mapgen();

  set_bkg_submap(0, 0, 16, 16, tmap + 19, 18); // TODO: buggy w/ non-zero x, y

  LCDC_REG = 0b10000001;  // display on, bg on
  enable_interrupts();

  while(1) {
    key = joypad();

    // XXX
    if (key & J_A) {
      disable_interrupts();
      DISPLAY_OFF;
      mapgen();
      set_bkg_submap(0, 0, 16, 16, tmap + 19, 18);
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
}

void roomgen(void) {
  Region *region, *old_region_end;
  u8 failmax = 5, maxwidth = 10, maxheight = 10, maxh;
  u8 x, y, w, h;
  u8 regl, regr, regt, regb, regw, regh;
  u8 rooml, roomr, roomt, roomb;
  u16 pos;

  blankmap(tmap, 255, 2);
  memset(roommap, 0, sizeof(roommap));
  blankmap(sigmap, 255, 255);
  blankmap(ds_sets, 0, 0);

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
      pos = getpos(x, y);
      mapset(tmap + pos, w, h, 1);
      mapset(roommap + pos, w, h, ++num_rooms);
      sigrect_empty(sigmap + pos, w, h);

      // Update disjoint set regions; we know that they're disjoint so they
      // each get their own region id (i.e. the room number)
      mapset(ds_sets + pos, w, h, ++ds_nextid);
      ds_parents[ds_nextid] = ds_nextid;
      ds_sizes[ds_nextid] = w * h; // XXX multiply
      ++ds_num_sets;

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
  u16 pos;
  u8 cand;

  memset(tempmap, 0, sizeof(tempmap));

  // Find all candidates for worm start position
  num_cands = 0;
  pos = 19;
  do {
    update_carve1(pos);
  } while(++pos < 305);

  do {
    if (num_cands) {
      // Choose a random start, and calculate its offset
      cand = randint(num_cands);
      pos = 19;
      do {
        if (tempmap[pos] == 2 && cand-- == 0) { break; }
      } while(++pos < 305);

      // Update the disjoint set with this new region.
      ++ds_nextid;
      ++ds_num_sets;
      ds_parents[ds_nextid] = ds_nextid;
      ds_sizes[ds_nextid] = 0;

      digworm(pos);

      // Only the first spot may be connected to another empty tile. No other
      // tile can be connect to another region (since it would not be
      // carvable). So we only need to merge regions here.
      ds_union(ds_nextid, ds_sets[pos - 18]);
      ds_union(ds_nextid, ds_sets[pos - 1]);
      ds_union(ds_nextid, ds_sets[pos + 1]);
      ds_union(ds_nextid, ds_sets[pos + 18]);
    }
  } while(num_cands > 1);
}

void digworm(u16 pos) {
  u8 dir, step, num_dirs;

  // Pick a random direction
  dir = rand() & 3;
  step = 0;
  do {
    tmap[pos] = 1;          // Set tile to empty.
    sigempty(sigmap + pos); // Update neighbor signatures.

    // Remove this tile from the carvable map
    if (tempmap[pos] == 2) { --num_cands; }
    tempmap[pos] = 0;

    // Update neighbors
    update_carve1(pos-19);
    update_carve1(pos-18);
    update_carve1(pos-17);
    update_carve1(pos-1);
    update_carve1(pos+1);
    update_carve1(pos+17);
    update_carve1(pos+18);
    update_carve1(pos+19);

    // Update the disjoint set
    ds_sets[pos] = ds_nextid;
    ++ds_sizes[ds_nextid];

    // Continue in the current direction, unless we can't carve. Also randomly
    // change direction after 3 or more steps.
    if (!(tempmap[pos + dirpos[dir]]) || ((rand() & 1) && step > 2)) {
      step = 0;
      num_dirs = 0;
      for (dir = 0; dir < 4; ++dir) {
        if (tempmap[pos + dirpos[dir]]) {
          cands[num_dirs++] = dir;
        }
      }
      if (num_dirs == 0) return;
      dir = cands[randint(num_dirs)];
    }
    pos += dirpos[dir];
    ++step;
  } while(1);
}

u8 nexttoroom(u16 pos) {
  return roommap[pos - 18] || roommap[pos - 1] || roommap[pos + 1] ||
         roommap[pos + 18];
}

void update_carve1(u16 pos) {
  u8 result;
  u8 tile = tmap[pos];
  if ((flags_bin[tile] & 1) && tile != 255 &&
      sigmatch(pos, carvesig, carvemask, sizeof(carvesig))) {
    result = 1;
    if (!nexttoroom(pos)) {
      ++result;
      if (tempmap[pos] != 2) { ++num_cands; }
    }
  } else {
    result = 0;
    if (tempmap[pos] == 2) { --num_cands; }
  }
  tempmap[pos] = result;
}

void carvedoors(void) {
  u16 pos;
  u8 cand, match, diff;

  pos = 19;
  num_cands = 0;
  do {
    update_door1(pos);
  } while(++pos < 305);

  do {
    // Pick a random door, and calculate its position.
    cand = randint(num_cands);
    pos = 19;

    do {
      if (tempmap[pos] && cand-- == 0) { break; }
    } while (++pos < 305);

    // Merge the regions, if possible. They may be already part of the same
    // set, in which case they should not be joined.
    match = sigmatch(pos, doorsig, doormask, sizeof(doorsig));
    diff = match == 1 ? 18 : 1; // 1: horizontal, 2: vertical
    if (ds_union(ds_sets[pos - diff], ds_sets[pos + diff])) {
      // Insert an empty tile to connect the regions
      ds_sets[pos] = ds_sets[pos + diff];
      tmap[pos] = 1;
      sigempty(sigmap + pos); // Update neighbor signatures.

      // Remove this tile from the carvable map
      --num_cands;
      tempmap[pos] = 0;

      // Update neighbors
      update_door1(pos-18);
      update_door1(pos-1);
      update_door1(pos+1);
      update_door1(pos+18);
    } else {
      tempmap[pos] = 0;
      --num_cands;
    }
  } while (num_cands);
}

void update_door1(u16 pos) {
  u8 tile = tmap[pos], diff;
  // A door connects two regions in the following patterns:
  //   * 0 *   * 1 *
  //   1   1   0   0
  //   * 0 *   * 1 *
  //
  // Where 1s are walls and 0s are not. The `match` index returns whether it is
  // a horizontal door or vertical. A door is only allowed between two regions
  // that are not already connected. We use the disjoint set to determine this
  // quickly below.
  u8 match = sigmatch(pos, doorsig, doormask, sizeof(doorsig));
  u8 result = (tile != 255 && (flags_bin[tile] & 1) && match);
  if (result) {
    diff = match == 1 ? 18 : 1; // 1: horizontal, 2: vertical
    if (ds_find(ds_sets[pos + diff]) == ds_find(ds_sets[pos - diff])) {
      result = 0;
      if (tempmap[pos]) {
        --num_cands;
      }
    } else if (!tempmap[pos]) {
      ++num_cands;
    }
  }
  tempmap[pos] = result;
}

void carvecuts(void) {
  u16 pos;
  u8 tile, match, result, cand;

  pos = 19;
  do {
    tile = tmap[pos];
    match = sigmatch(pos, doorsig, doormask, sizeof(doorsig));
    result = (tile != 255 && (flags_bin[tile] & 1) && match);
    if (result) {
      ++num_cands;
    }
    tempmap[pos] = result;
  } while(++pos < 305);

  // Pick a random cut, and calculate its position.
  cand = randint(num_cands);
  pos = 19;

  do {
    if (tempmap[pos] && cand-- == 0) { break; }
  } while (++pos < 305);

  tmap[pos] = 32;

  // TODO: how to calculate the distance table?
}

void append_region(u8 x, u8 y, u8 w, u8 h) {
  new_region_end->x = x;
  new_region_end->y = y;
  new_region_end->w = w;
  new_region_end->h = h;
  ++new_region_end;
}

void blankmap(u8* map, u8 border, u8 def) {
  memset(map, border, 18 * 18);  // set border
  mapset(map + 19, 16, 16, def); // set inner
}

void mapset(u8* pos, u8 w, u8 h, u8 val) {
  u8 w2 = w;
  do {
    *pos++ = val;
    if (--w2 == 0) {
      w2 = w;
      --h;
      pos += 18 - w;
    }
  } while (h);
}

void sigrect_empty(u8* pos, u8 w, u8 h) {
  u8 w2 = w;
  do {
    sigempty(pos);
    ++pos;

    if (--w2 == 0) {
      w2 = w;
      --h;
      pos += 18 - w;
    }
  } while (h);
}

void sigempty(u8* pos) {
  pos[-19] &= 0b11111011;
  pos[-18] &= 0b11101111;
  pos[-17] &= 0b11111101;
  pos[-1]  &= 0b10111111;
  pos[+1]  &= 0b01111111;
  pos[+17] &= 0b11110111;
  pos[+18] &= 0b11011111;
  pos[+19] &= 0b11111110;
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

u16 getpos(u8 x, u8 y) {
  u16 result = 0;
  do { result += 18; } while (--y);
  do { ++result; } while (--x);
  return result;
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

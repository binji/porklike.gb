#include <gb/gb.h>

#include "animate.h"

#include "gameplay.h"
#include "mob.h"
#include "pickup.h"
#include "spr.h"
#include "sprite.h"

// TODO: how big to make these arrays?
#define DIRTY_SIZE 128
#define DIRTY_CODE_SIZE 512
#define ANIM_TILES_SIZE 64

#define TILE_ANIM_FRAMES 8
#define TILE_ANIM_FRAME_DIFF 16

u8 dirty[DIRTY_SIZE];
u8 dirty_code[DIRTY_CODE_SIZE];
u8 *dirty_ptr;

u8 dirty_saw[DIRTY_SIZE];
u8 *dirty_saw_ptr;

u8 anim_tiles[ANIM_TILES_SIZE];
u8 *anim_tile_ptr;
u8 anim_tile_timer; // timer for animating tiles (every TILE_ANIM_FRAMES frames)

void animate_init(void) {
  dirty_ptr = dirty;
  dirty_code[0] = 0xc9;
  anim_tile_ptr = anim_tiles;
  anim_tile_timer = TILE_ANIM_FRAMES;
}

void animate_begin(void) {
  dirty_ptr = dirty;
  sprite_animate_begin();
}

void animate_end(void) {
  sprite_animate_end();

  // Process dirty tiles
  u8* ptr = dirty;
  u16 last_addr = 0;
  u8 last_val = 0;

  // Keep track of updated saws (they need a different palette)
  dirty_saw_ptr = dirty_saw;

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
      if ((tile & TILE_SAW_MASK) == TILE_SAW || tile == TILE_SAW_BROKEN) {
        *dirty_saw_ptr++ = pos;
      }
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

#ifdef CGB_SUPPORT
  if (_cpu == CGB_TYPE && dirty_saw_ptr != dirty_saw) {
    // CGB needs to update the palette data too, but just for saws: animated
    // saws use pal 1, and broken saws use pal 0.
    u8 *ptr = dirty_saw;
    u16 last_addr = 0;
    u8 last_val = 1;

    *code++ = 0x3e; // ld a, 1
    *code++ = 1;
    *code++ = 0xe0; // ldh 0xff4f, a (set vram bank to 1)
    *code++ = 0x4f; //

    while (ptr != dirty_saw_ptr) {
      u8 pos = *ptr++;
      u8 pal;
      u16 addr;

      pal = tmap[pos] != TILE_SAW_BROKEN;
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
      if (pal != last_val) {
        *code++ = pal == 0 ? 0x3d : 0x3c; // inc/dec a
        last_val = pal;
      }
      *code++ = 0x77; // ld (hl), a
    }

    // Switch back to vram bank 0
    *code++ = 0xaf; // xor a
    *code++ = 0xe0; // ldh 0xff4f, a (set vram bank to 0)
    *code++ = 0x4f; //
  }
#endif

  *code++ = 0xf1;       // pop af
  *code++ = 0xc9;       // ret
  dirty_code[0] = 0xf5; // push af
}

u8 animate(void) {
  u8 animdone = num_sprs == 0;

  // Loop through all animating tiles
  if (--anim_tile_timer == 0) {
    anim_tile_timer = TILE_ANIM_FRAMES;
    u8 *ptr = anim_tiles;
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


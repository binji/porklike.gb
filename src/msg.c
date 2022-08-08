#include <gb/gb.h>
#include <string.h>

#include "msg.h"

#include "inventory.h"
#include "sprite.h"

#pragma bank 1

#define MAX_MSG_SPRITES 18
#define MSG_FRAMES 120
#define MSG_INFINITE_FRAMES 255

#define MSG_WURSTCHAIN 36
#define MSG_WURSTCHAIN_Y 24
#define TILE_WURSTCHAIN_1 0x79

extern const u8 msg_tiles[];

OAM_item_t msg_sprites[MAX_MSG_SPRITES];
u8 msg_timer;
u8 msg_y;
u8 msg_sprite_bytes;

void msg_hide(void) { msg_timer = 0; }

void msg_show(u8 index, u8 y) {
  u8 i, j;
  u8* p = (u8*)msg_sprites;
  for (j = 0; j < 2; ++j) {
    for (i = 0; i < 9; ++i) {
      *p++ = y;
      *p++ = 52 + (i << 3);
      *p++ = msg_tiles[index++];
      *p++ = 5;
    }
    y += 8;
  }
  msg_timer = MSG_FRAMES;
  msg_sprite_bytes = MAX_MSG_SPRITES * 4;
  msg_y = y;
}

void msg_wurstchain(u8 chain, u8 prop) {
  u8 index = MSG_WURSTCHAIN;
  u8 i, j;
  u8* p = (u8*)msg_sprites;
  u8 y = MSG_WURSTCHAIN_Y;

  msg_timer = MSG_INFINITE_FRAMES;
  msg_sprite_bytes = 13 * 4;
  msg_y = MSG_WURSTCHAIN_Y;

  for (j = 0; j < 2; ++j) {
    for (i = 0; i < 6; ++i) {
      *p++ = y;
      *p++ = 62 + (i << 3);
      *p++ = msg_tiles[index++];
      *p++ = prop;
    }
    y += 8;
  }

  msg_sprites[8].x -= 6;
  msg_sprites[9].x += 2;
  msg_sprites[11].x -= 3;

  msg_sprites[12].x = 62 + (2 << 3) + 2;
  msg_sprites[12].y = MSG_WURSTCHAIN_Y + 8;
  msg_sprites[12].tile = chain + TILE_WURSTCHAIN_1 - 1;
  msg_sprites[12].prop = prop;
}

void msg_update(void) {
  // Display message sprites, if any
  if (msg_timer) {
    if ((msg_timer == MSG_INFINITE_FRAMES || --msg_timer != 0) &&
        msg_y <= INV_TOP_Y()) {
      memcpy(next_sprite, (void *)msg_sprites, msg_sprite_bytes);
      next_sprite += msg_sprite_bytes;
    }
  }
}


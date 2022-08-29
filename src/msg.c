#include <gb/gb.h>
#include <string.h>

#include "msg.h"

#include "inventory.h"
#include "sprite.h"

#pragma bank 1

#define MAX_MSG_SPRITES 18
#define MSG_FRAMES 120

extern const u8 msg_tiles[];

OAM_item_t msg_sprites[MAX_MSG_SPRITES];
u8 msg_timer;

void msg_hide(void) { msg_timer = 0; }

void msg_show(u8 index, u8 y) {
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

void msg_update(void) {
  // Display message sprites, if any
  if (msg_timer) {
    if (--msg_timer != 0 && MSG_REAPER_Y <= INV_TOP_Y()) {
      memcpy(next_sprite, (void *)msg_sprites, MAX_MSG_SPRITES * 4);
      next_sprite += MAX_MSG_SPRITES * 4;
    }
  }
}


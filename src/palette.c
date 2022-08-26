#include <gb/gb.h>

#include "common.h"
#include "palette.h"

#pragma bank 1

#define FADE_FRAMES 8
#define OBJ1_PAL_FRAMES 8

extern const u8 fadepal[];
extern const u8 obj_pal1[];

static u8 obj_pal1_timer, obj_pal1_index;

void pal_init(void) {
  // 0:Black     1:Black    2:Black 3:Black
  BGP_REG = OBP0_REG = OBP1_REG = 0b11111111;
  obj_pal1_timer = OBJ1_PAL_FRAMES;
}

void pal_update(void) {
  if (--obj_pal1_timer == 0) {
    obj_pal1_timer = OBJ1_PAL_FRAMES;
    OBP1_REG = obj_pal1[obj_pal1_index = (obj_pal1_index + 1) & 7];
  }
}

void pal_fadeout(void) NONBANKED {
  u8 i, j;
  i = 3;
  do {
    for (j = 0; j < FADE_FRAMES; ++j) { wait_vbl_done(); }
    BGP_REG = OBP0_REG = OBP1_REG = fadepal[i];
  } while(i--);
}

void pal_fadein(void) NONBANKED {
  u8 i, j;
  i = 0;
  do {
    for (j = 0; j < FADE_FRAMES; ++j) { wait_vbl_done(); }
    BGP_REG = OBP0_REG = OBP1_REG = fadepal[i];
  } while(++i != 4);
}

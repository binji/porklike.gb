#include <gb/gb.h>
#include <gb/gbdecompress.h>

#include "main.h"

#pragma bank 2

#define TITLE_X_OFFSET 2
#define TITLE_Y_OFFSET 2
#define TITLE_WIDTH 16
#define TITLE_HEIGHT 14

#include "tiletitle.c"

const u8 title_map[] = {
    14, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
    14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 14,
    14, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 14,
    14, 14, 14, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 14, 14, 14,
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
    14, 14, 14, 83, 84, 85, 86, 86, 14, 87, 88, 89, 14, 14, 14, 14,
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
    53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68,
    14, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 14,
};

void titlescreen(void) {
  gb_decompress_bkg_data(0, title_bin);
  init_bkg(0xe);
  set_bkg_tiles(TITLE_X_OFFSET, TITLE_Y_OFFSET, TITLE_WIDTH, TITLE_HEIGHT,
                title_map);
  BGP_REG = fadepal[3];
  LCDC_REG = 0b10000011;  // display on, bg/obj on,

  while(1) {
    lastjoy = joy;
    joy = joypad();
    newjoy = (joy ^ lastjoy) & joy;

    if (newjoy & J_START) {
      break;
    }
    wait_vbl_done();
  }

  fadeout();
}

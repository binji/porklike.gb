#include <gb/gb.h>
#include <gb/cgb.h>

#include "common.h"
#include "palette.h"

#pragma bank 1

#define FADE_FRAMES 8
#define FLASH_PAL_FRAMES 8

extern const u8 fadepal[];
extern const u8 obj_pal1[];

extern const palette_color_t cgb_bkg_pals[], cgb_obp_pals[];
extern const palette_color_t cgb_player_dmg_flash_pals[];
extern const palette_color_t cgb_mob_key_flash_pals[];
extern const palette_color_t cgb_press_start_flash_pals[];

static u8 flash_pal_timer, flash_pal_index;
static u8 saved_bkg_pal[64], saved_obp_pal[64];

void pal_init(void) {
  if (_cpu == CGB_TYPE) {
    cpu_fast();
    set_bkg_palette(0, 5, cgb_bkg_pals);
    set_sprite_palette(0, 7, cgb_obp_pals);
  } else {
    // 0:Black     1:Black    2:Black 3:Black
    BGP_REG = OBP0_REG = OBP1_REG = 0b11111111;
  }
  flash_pal_timer = FLASH_PAL_FRAMES;
}

void pal_update(void) {
  if (--flash_pal_timer == 0) {
    flash_pal_timer = FLASH_PAL_FRAMES;
    flash_pal_index = (flash_pal_index + 1) & 7;
    if (_cpu == CGB_TYPE) {
      set_sprite_palette(2, 1,
                         &cgb_player_dmg_flash_pals[flash_pal_index << 2]);
      set_sprite_palette(4, 1, &cgb_mob_key_flash_pals[flash_pal_index << 2]);
      set_bkg_palette(4, 1, &cgb_press_start_flash_pals[flash_pal_index << 2]);
    } else {
      OBP1_REG = obj_pal1[flash_pal_index];
    }
  }
}

u16 next_fadeout_color(u16 color) NONBANKED {
  u16 newcolor = (color | 0b1000010000100000) - 0b0000100001000010;
  u16 overflow = newcolor & 0b1000010000100000;
  overflow -= overflow >> 5;
  newcolor &= overflow;
  return newcolor;
}

void pal_fadeout(void) NONBANKED {
  u8 i, j;
  if (_cpu == CGB_TYPE) {
    u16 color;

    // Save original palette.
    for (i = 0; i < 64; ++i) {
      rBCPS = i;
      saved_bkg_pal[i] = rBCPD;
      rOCPS = i;
      saved_obp_pal[i] = rOCPD;
    }

    i = FADE_FRAMES * 3;
    do {
      for (j = 0; j < 64; j += 2) {
        rBCPS = j + 1;
        color = rBCPD << 8;
        rBCPS = j | BCPSF_AUTOINC;
        color |= rBCPD;
        color = next_fadeout_color(color);
        rBCPD = color & 0xff;
        rBCPD = color >> 8;

        rOCPS = j + 1;
        color = rOCPD << 8;
        rOCPS = j | OCPSF_AUTOINC;
        color |= rOCPD;
        color = next_fadeout_color(color);
        rOCPD = color & 0xff;
        rOCPD = color >> 8;
      }
      wait_vbl_done();
    } while (i--);
  } else {
    i = 3;
    do {
      for (j = 0; j < FADE_FRAMES; ++j) { wait_vbl_done(); }
      BGP_REG = OBP0_REG = OBP1_REG = fadepal[i];
    } while (i--);
  }
}

u16 next_fadein_color(u16 src, u16 dst) NONBANKED {
  u16 newcolor = (src & 0b0111101111011111) + 0b0000100001000010;
  u16 overflow = ((dst & 0b0111101111011111) - newcolor) & 0b1000010000100000;
  overflow -= overflow >> 5;
  newcolor = (dst & overflow) | (newcolor & ~overflow);
  return newcolor;
}

void pal_fadein(void) NONBANKED {
  u8 i, j;
  if (_cpu == CGB_TYPE) {
    u16 color;
    i = FADE_FRAMES * 3;
    do {
      for (j = 0; j < 64; j += 2) {
        rBCPS = j + 1;
        color = rBCPD << 8;
        rBCPS = j | BCPSF_AUTOINC;
        color |= rBCPD;
        color = next_fadein_color(color, *((u16 *)(saved_bkg_pal + j)));
        rBCPD = color & 0xff;
        rBCPD = color >> 8;

        rOCPS = j + 1;
        color = rOCPD << 8;
        rOCPS = j | OCPSF_AUTOINC;
        color |= rOCPD;
        color = next_fadein_color(color, *((u16 *)(saved_obp_pal + j)));
        rOCPD = color & 0xff;
        rOCPD = color >> 8;
      }
      wait_vbl_done();
    } while (i--);
  } else {
    i = 0;
    do {
      for (j = 0; j < FADE_FRAMES; ++j) { wait_vbl_done(); }
      BGP_REG = OBP0_REG = OBP1_REG = fadepal[i];
    } while (++i != 4);
  }
}

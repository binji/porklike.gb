#include <gb/gb.h>
#include <gb/cgb.h>
#include <string.h>

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
static u16 saved_bkg_pal[32], saved_obp_pal[32];
static u16 cur_bkg_pal[32], cur_obp_pal[32];

void pal_init(void) {
#ifdef CGB_SUPPORT
  if (_cpu == CGB_TYPE) {
    cpu_fast();
    set_bkg_palette(0, 5, cgb_bkg_pals);
    set_sprite_palette(0, 7, cgb_obp_pals);
  } else
#endif
  {
    // 0:Black     1:Black    2:Black 3:Black
    BGP_REG = OBP0_REG = OBP1_REG = 0b11111111;
  }
  flash_pal_timer = FLASH_PAL_FRAMES;
}

void pal_update(void) {
  if (--flash_pal_timer == 0) {
    flash_pal_timer = FLASH_PAL_FRAMES;
    flash_pal_index = (flash_pal_index + 1) & 7;
#ifdef CGB_SUPPORT
    if (_cpu == CGB_TYPE) {
      set_sprite_palette(2, 1,
                         &cgb_player_dmg_flash_pals[flash_pal_index << 2]);
      set_sprite_palette(4, 1, &cgb_mob_key_flash_pals[flash_pal_index << 2]);
      set_bkg_palette(4, 1, &cgb_press_start_flash_pals[flash_pal_index << 2]);
    } else
#endif
    {
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

void get_bkg_palettes(u8* dest);
void get_obp_palettes(u8* dest);

void pal_fadeout(void) NONBANKED {
  u8 i, j;
#ifdef CGB_SUPPORT
  if (_cpu == CGB_TYPE) {
    // Save original palette.
    get_bkg_palettes(saved_bkg_pal);
    get_obp_palettes(saved_obp_pal);

    // Update cur palette.
    memcpy(cur_bkg_pal, saved_bkg_pal, sizeof(cur_bkg_pal));
    memcpy(cur_obp_pal, saved_obp_pal, sizeof(cur_obp_pal));

    i = FADE_FRAMES * 3;
    do {
      // Calculate next palette.
      for (j = 0; j < 32; ++j) {
        cur_bkg_pal[j] = next_fadeout_color(cur_bkg_pal[j]);
        cur_obp_pal[j] = next_fadeout_color(cur_obp_pal[j]);
      }

      // Write it.
      set_bkg_palette(0, 8, cur_bkg_pal);
      set_sprite_palette(0, 8, cur_obp_pal);
      wait_vbl_done();
    } while (i--);
  } else
#endif
  {
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
#ifdef CGB_SUPPORT
  if (_cpu == CGB_TYPE) {
    i = FADE_FRAMES * 3;
    do {
      u16 *saved_bkg = saved_bkg_pal;
      u16 *saved_obp = saved_obp_pal;
      // Calculate next palette.
      for (j = 0; j < 32; ++j) {
        cur_bkg_pal[j] = next_fadein_color(cur_bkg_pal[j], saved_bkg[j]);
        cur_obp_pal[j] = next_fadein_color(cur_obp_pal[j], saved_obp[j]);
      }

      // Write it.
      set_bkg_palette(0, 8, cur_bkg_pal);
      set_sprite_palette(0, 8, cur_obp_pal);
      wait_vbl_done();
    } while (i--);
  } else
#endif
  {
    i = 0;
    do {
      for (j = 0; j < FADE_FRAMES; ++j) { wait_vbl_done(); }
      BGP_REG = OBP0_REG = OBP1_REG = fadepal[i];
    } while (++i != 4);
  }
}

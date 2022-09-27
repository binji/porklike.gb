#include <gb/gb.h>
#include <gb/gbdecompress.h>

#include "main.h"

#include "animate.h"
#include "counter.h"
#include "float.h"
#include "gameover.h"
#include "gameplay.h"
#include "inventory.h"
#include "joy.h"
#include "mob.h"
#include "palette.h"
#include "pickup.h"
#include "rand.h"
#include "sound.h"
#include "targeting.h"

#pragma bank 1

#include "tileshared.h"
#include "tilesprites.h"

void titlescreen(void);
void vbl_interrupt(void);

u8 floor;

void main(void) NONBANKED {
  // Initialize for title screen
  DISPLAY_OFF;
  clear_wram();
  sram_init();
  pal_init();
  soundinit();
  enable_interrupts();

  SWITCH_ROM_MBC1(2);
  titlescreen();
  SWITCH_ROM_MBC1(1);

  // Initialize for gameplay
  init_win(0);
  LCDC_REG = LCDCF_ON | LCDCF_WINON | LCDCF_BGON | LCDCF_OBJON | LCDCF_WIN9C00;
  gb_decompress_bkg_data(0x80, tileshared_bin);
  gb_decompress_sprite_data(0, tilesprites_bin);
  add_VBL(vbl_interrupt);
  gameplay_init();
  doloadfloor = 1;

  while(1) {
    joy_update();

    // On floor 0, try to gather more entropy from player inputs.
    if (floor == 0 && newjoy) {
      xrnd_mix(joy);
      xrnd_mix(DIV_REG);
    }

    animate_begin();

    if (gameover_state != GAME_OVER_NONE) {
      gameover_update();
    } else {
      gameplay_update();
    }
    wait_vbl_done();
  }
}

void vbl_interrupt(void) NONBANKED {
  if (doupdatemap) {
    set_bkg_tiles(0, 0, MAP_WIDTH, MAP_HEIGHT, dtmap);
    doupdatemap = 0;
  }
  ((vfp)dirty_code)();
}


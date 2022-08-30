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

#include "tilebg.h"
#include "tileshared.h"
#include "tilesprites.h"

void titlescreen(void);
void gameinit(void);
void vbl_interrupt(void);

u8 floor;

void main(void) NONBANKED {
  // Initialize for title screen
  DISPLAY_OFF;
  clear_wram();
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
  gameinit();
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

void gameinit(void) {
  animate_init();

  // Reset scroll registers
  move_bkg(240, 0);
  // Reload bg tiles
  gb_decompress_bkg_data(0, tilebg_bin);
  init_bkg(0);

  // Reset player mob
  num_mobs = 0;
  addmob(MOB_TYPE_PLAYER, 0);
  inv_update_hp();

  turn = TURN_PLAYER;
  float_hide();

  // Set up inventory window
  move_win(23, 128);
  inv_init();
  targeting_init();

  floor = 0;
  counter_zero(&st_floor);
  counter_zero(&st_steps);
  counter_zero(&st_kills);
  joy_init();
}

void vbl_interrupt(void) NONBANKED {
  if (doupdatemap) {
    set_bkg_tiles(0, 0, MAP_WIDTH, MAP_HEIGHT, dtmap);
    doupdatemap = 0;
  }
  ((vfp)dirty_code)();
}


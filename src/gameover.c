#include <gb/gb.h>
#include <gb/gbdecompress.h>

#include "gameover.h"

#include "animate.h"
#include "counter.h"
#include "gameplay.h"
#include "joy.h"
#include "palette.h"
#include "sound.h"
#include "sprite.h"

#pragma bank 1

#include "tiledead.h"

#define DEAD_X_OFFSET 4
#define DEAD_Y_OFFSET 2
#define DEAD_WIDTH 8
#define DEAD_HEIGHT 5

#define WIN_X_OFFSET 2
#define WIN_Y_OFFSET 2
#define WIN_WIDTH 12
#define WIN_HEIGHT 6

#define TEXT_X_OFFSET 2
#define TEXT_Y_OFFSET 6
#define TEXT_WIDTH 12
#define TEXT_HEIGHT 2

#define PRESS_A_X_OFFSET 4
#define PRESS_A_Y_OFFSET 14
#define PRESS_A_WIDTH 7
#define PRESS_A_HEIGHT 1

#define STATS_X_OFFSET 3
#define STATS_Y_OFFSET 9
#define STATS_WIDTH 9
#define STATS_HEIGHT 6
#define STATS_COLOR_WIDTH 14
#define STATS_COLOR_HEIGHT 3

#define STATS_FLOOR_ADDR 0x992c
#define STATS_STEPS_ADDR 0x994c
#define STATS_KILLS_ADDR 0x996c

void clear_bkg(void);

extern const u8 dead_map[];
extern const u8 win_map[];
extern const u8 stats_map[];

GameOverState gameover_state;
u8 gameover_timer;

void gameover_update(void) {
  if (gameover_state != GAME_OVER_WAIT) {
    animate_end();
    sprite_hide();
    pal_fadeout();
    IE_REG &= ~VBL_IFLAG;

    // Hide window
    move_win(23, 160);
    gb_decompress_bkg_data(0, tiledead_bin);
    clear_bkg();
    if (gameover_state == GAME_OVER_WIN) {
      music_win();
      set_bkg_tiles(WIN_X_OFFSET, WIN_Y_OFFSET, WIN_WIDTH, WIN_HEIGHT, win_map);
      if (wurstchain < MAX_WURSTCHAIN) {
        wurstchain += 1;
        sram_update_wurstchain(wurstchain);
      }
    } else {
      music_dead();
      set_bkg_tiles(DEAD_X_OFFSET, DEAD_Y_OFFSET, DEAD_WIDTH, DEAD_HEIGHT,
                    dead_map);
      wurstchain = 0;
    }
    set_bkg_tiles(STATS_X_OFFSET, STATS_Y_OFFSET, STATS_WIDTH, STATS_HEIGHT,
                  stats_map);
    counter_out(&st_floor, STATS_FLOOR_ADDR);
    counter_out(&st_steps, STATS_STEPS_ADDR);
    counter_out(&st_kills, STATS_KILLS_ADDR);

#ifdef CGB_SUPPORT
    if (_cpu == CGB_TYPE) {
      VBK_REG = 1;
      // Set the palette for the text rows.
      fill_bkg_rect(TEXT_X_OFFSET, TEXT_Y_OFFSET, TEXT_WIDTH, TEXT_HEIGHT, 3);
      // Set the palette for the stats rows.
      fill_bkg_rect(STATS_X_OFFSET, STATS_Y_OFFSET, STATS_COLOR_WIDTH,
                    STATS_COLOR_HEIGHT, 2);
      // Set the palette for the "press a" row.
      fill_bkg_rect(PRESS_A_X_OFFSET, PRESS_A_Y_OFFSET, PRESS_A_WIDTH,
                    PRESS_A_HEIGHT, 4);
      VBK_REG = 0;
    }
#endif

    gameover_state = GAME_OVER_WAIT;
    IE_REG |= VBL_IFLAG;
    pal_fadein();
  } else {
    // Wait for keypress
    if (newjoy & J_A) {
      sfx(SFX_OK);
      gameover_state = GAME_OVER_NONE;
      doloadfloor = 1;
      pal_fadeout();

      // reset initial state
      music_main();
      gameplay_init();
    } else {
      pal_update();
    }
  }
}

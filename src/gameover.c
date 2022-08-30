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

#define STATS_X_OFFSET 3
#define STATS_Y_OFFSET 9
#define STATS_WIDTH 9
#define STATS_HEIGHT 6

#define STATS_FLOOR_ADDR 0x992c
#define STATS_STEPS_ADDR 0x994c
#define STATS_KILLS_ADDR 0x996c

void gameinit(void);

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
    init_bkg(0);
    if (gameover_state == GAME_OVER_WIN) {
      music_win();
      set_bkg_tiles(WIN_X_OFFSET, WIN_Y_OFFSET, WIN_WIDTH, WIN_HEIGHT, win_map);
    } else {
      music_dead();
      set_bkg_tiles(DEAD_X_OFFSET, DEAD_Y_OFFSET, DEAD_WIDTH, DEAD_HEIGHT,
                    dead_map);
    }
    set_bkg_tiles(STATS_X_OFFSET, STATS_Y_OFFSET, STATS_WIDTH, STATS_HEIGHT,
                  stats_map);
    counter_out(&st_floor, STATS_FLOOR_ADDR);
    counter_out(&st_steps, STATS_STEPS_ADDR);
    counter_out(&st_kills, STATS_KILLS_ADDR);

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
      gameinit();
    }
  }
}

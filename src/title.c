#include <gb/gb.h>
#include <gb/gbdecompress.h>

#include "main.h"
#include "palette.h"
#include "rand.h"
#include "sound.h"
#include "spr.h"

#include "tiletitle.h"

#pragma bank 2

#define FADE_FRAMES 8
#define WAIT_FRAMES 30

#define TITLETOP_WIDTH 14
#define TITLETOP_HEIGHT 4

#define GAMEBOY_OBJS 12
#define GAMEBOY_Y 108

#define SLIDE_LAG 30

#define PRESS_START_OBJS 8
#define PRESS_START_X 52
#define PRESS_START_Y 92
#define PRESS_START_FRAMES 20

#define TITLEBOT_WIDTH 16
#define TITLEBOT_HEIGHT 2

const u8 titletop_map[] = {
    128,  129,  130,  131,  132,  133,  134,  135,  136,  137,  138, 139, 140, 141,
    143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156,
    157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170,
    142, 142, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 142, 142,
};

const u8 titlebot_map[] = {
    181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196,
    142, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 142,
};

const u8 titletop_bounce[] = {
    112, 112, 111, 109, 106, 103, 99,  94,  88,  82,  75,  67,  59,
    50,  40,  29,  17,  5,   248, 235, 220, 205, 193, 201, 208, 214,
    219, 224, 228, 231, 233, 235, 236, 236, 235, 234, 232, 229, 226,
    221, 216, 211, 204, 197, 193, 197, 199, 201, 203, 203, 203, 202,
    200, 197, 194, 193, 194, 195, 195, 194, 192};

const u8 bounce_sfx[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, SFX_HIT_MOB, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, SFX_HIT_MOB, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, SFX_HIT_MOB, 0, 0, 0, 0, SFX_HIT_MOB};

const u8 gameboy_tiles[] = {
    0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5,
};

const u8 gameboy_offset_x[] = {
    0x0, 0x8, 0x10, 0x18, 0x20, 0x28, 0x0, 0x8, 0x10, 0x18, 0x20, 0x28,
};

const u8 gameboy_offset_y[] = {
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
};

const u8 gameboy_elastic[] = {
    168, 151, 127, 101, 75, 54, 38, 29, 25, 27, 33, 42, 51, 60, 68, 73,
    77,  78,  77,  75,  72, 69, 65, 63, 61, 60, 59, 59, 60, 61, 62, 63,
    64,  65,  66,  66,  66, 65, 65, 65, 64, 64, 64, 63, 63, 63, 64, 64,
    64,  64,  64,  64,  64, 64, 64, 64, 64, 64, 64, 64, 64};

const u8 cubic_up[] = {0,   253, 250, 247, 244, 242, 239, 237, 235, 233,
                       231, 229, 227, 225, 224, 222, 221, 220, 218, 217,
                       216, 215, 214, 214, 213, 212, 212, 211, 211, 210,
                       210, 209, 209, 209, 209, 209, 208, 208, 208, 208,
                       208, 208, 208, 208, 208, 208};

const u8 press_start_tiles[] = {
    0xd3, 0xd4, 0xd5, 0xd6, 0xd6, 0xd7, 0xd8, 0xd9,
};

const u8 press_start_offset_x[] = {
    0x0,  0x08, 0x10, 0x18, 0x20, // PRESS
    0x30, 0x38, 0x40,             // START
};

const u8 titlefadepal[] = {
    0b00000000, // White
    0b01010100, // LightGray
    0b10101010, // DarkGray
    0b11111111, // Black
};

void titlefadein(void);
void waitframes(u8 frames);

void titlescreen(void) {
  u8 i, j;
  gb_decompress_bkg_data(0x80, tiletitle_bin);

  init_bkg(0x8e);
  LCDC_REG = LCDCF_ON | LCDCF_BGON | LCDCF_OBJON;

  // fade from white to black
  titlefadein();

  // wait on black
  waitframes(WAIT_FRAMES);
  music_main();

  // Load bkg tiles, set BGP
  SCX_REG = 232;
  SCY_REG = titletop_bounce[0];
  set_bkg_tiles(0, 0, TITLETOP_WIDTH, TITLETOP_HEIGHT, titletop_map);
#ifdef CGB_SUPPORT
  if (_cpu == CGB_TYPE) {
    VBK_REG = 1;
    fill_bkg_rect(0, 0, TITLETOP_WIDTH, TITLETOP_HEIGHT, 1);
    VBK_REG = 0;
  } else
#endif
  {
    // 0:LightGray 1:DarkGray 2:Black 3:White
    BGP_REG = OBP0_REG = OBP1_REG = 0b00111001;
  }

  // drop title down
  for (i = 0; i < sizeof(titletop_bounce); ++i) {
    SCY_REG = titletop_bounce[i];
    if (bounce_sfx[i] != 0) {
      sfx(bounce_sfx[i]);
    }
    wait_vbl_done();
  }

  // set up OBJ tiles
  for (i = 0; i < GAMEBOY_OBJS; ++i) {
    shadow_OAM[i].tile = gameboy_tiles[i];
    shadow_OAM[i].prop = 0;
  }

  waitframes(WAIT_FRAMES);

  // slide gameboy in from the right
  for (i = 0; i < sizeof(gameboy_elastic); ++i) {
    for (j = 0; j < GAMEBOY_OBJS; ++j) {
      shadow_OAM[j].x = gameboy_elastic[i] + gameboy_offset_x[j];
      shadow_OAM[j].y = GAMEBOY_Y + gameboy_offset_y[j];
    }
    if (i == 13) {
      sfx(SFX_OPEN_OBJECT);
    }
    wait_vbl_done();
  }

  waitframes(WAIT_FRAMES);

  WX_REG = 16 + 7;
  WY_REG = 160;
  LCDC_REG = LCDCF_ON | LCDCF_WINON | LCDCF_BGON | LCDCF_OBJON | LCDCF_WIN9C00;
  init_win(0x8e);
  set_win_tiles(0, 0, TITLEBOT_WIDTH, TITLEBOT_HEIGHT, titlebot_map);
#ifdef CGB_SUPPORT
  if (_cpu == CGB_TYPE) {
    VBK_REG = 1;
    fill_win_rect(0, 0, TITLEBOT_WIDTH, TITLEBOT_HEIGHT, 3);
    VBK_REG = 0;
  }
#endif

  // slide up title, gameboy, and credits
  for (i = 0; i < sizeof(cubic_up) + SLIDE_LAG; ++i) {
    // slide up gameboy
    if (i < sizeof(cubic_up)) {
      for (j = 0; j < GAMEBOY_OBJS; ++j) {
        shadow_OAM[j].x = 64 + gameboy_offset_x[j];
        shadow_OAM[j].y = cubic_up[i] + GAMEBOY_Y + gameboy_offset_y[j];
      }
    }

    wait_vbl_done();

    // slide up porklike
    if (i < sizeof(cubic_up)) {
      SCY_REG = 192 - cubic_up[i];
    }

    // slide up credits lagged
    if (i >= SLIDE_LAG) {
      WY_REG = 160 + cubic_up[i - SLIDE_LAG];
    }
  }

  // Set up "press start" OBJS
  for (i = 0; i < PRESS_START_OBJS; ++i) {
    shadow_OAM[GAMEBOY_OBJS + i].x = PRESS_START_X + press_start_offset_x[i];
    shadow_OAM[GAMEBOY_OBJS + i].y = PRESS_START_Y;
    shadow_OAM[GAMEBOY_OBJS + i].tile = press_start_tiles[i];
    shadow_OAM[GAMEBOY_OBJS + i].prop = 6;
  }

  u8 ps_frames = PRESS_START_FRAMES;
  u8 ps_display = 0;

  // Wait for start button
  joy = joypad();
  while(1) {
    lastjoy = joy;
    joy = joypad();
    newjoy = (joy ^ lastjoy) & joy;

    if (--ps_frames == 0) {
      ps_frames = PRESS_START_FRAMES;
      ps_display ^= 1;

      for (i = 0; i < PRESS_START_OBJS; ++i) {
        shadow_OAM[GAMEBOY_OBJS + i].y = ps_display ? PRESS_START_Y : 160;
      }
    }

    if (newjoy & J_START) {
      sfx(SFX_OK);
      // Not much entropy, but it's OK -- we'll generate more on floor 0.
      xrnd_init(DIV_REG);
      break;
    }
    wait_vbl_done();
  }

  pal_fadeout();
  spr_hide();
}

void titlefadein(void) {
  u8 i;
  i = 0;
  do {
    BGP_REG = OBP0_REG = OBP1_REG = titlefadepal[i];
    waitframes(FADE_FRAMES);
  } while(++i != 4);
}

void waitframes(u8 frames) {
  u8 j;
  for (j = 0; j < frames; ++j) { wait_vbl_done(); }
}

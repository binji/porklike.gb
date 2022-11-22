#include <gb/cgb.h>

#include "inventory.h"
#include "main.h"
#include "mob.h"

// Only const data needs to go in here, since functions can be marked with
// NONBANKED to put them in bank 0.

#include "flags.c"

const u8 permutation_4[] = {
    0, 1, 2, 3,
    0, 1, 3, 2,
    0, 2, 1, 3,
    0, 2, 3, 1,
    0, 3, 1, 2,
    0, 3, 2, 1,
    1, 0, 2, 3,
    1, 0, 3, 2,
    1, 2, 0, 3,
    1, 2, 3, 0,
    1, 3, 0, 2,
    1, 3, 2, 0,
    2, 0, 1, 3,
    2, 0, 3, 1,
    2, 1, 0, 3,
    2, 1, 3, 0,
    2, 3, 0, 1,
    2, 3, 1, 0,
    3, 0, 1, 2,
    3, 0, 2, 1,
    3, 1, 0, 2,
    3, 1, 2, 0,
    3, 2, 0, 1,
    3, 2, 1, 0,
};

const u8 fadepal[] = {
    0b11111111, // 0:Black     1:Black    2:Black 3:Black
    0b10111111, // 0:Black     1:Black    2:Black 3:DarkGray
    0b01111110, // 0:DarkGray  1:Black    2:Black 3:LightGray
    0b00111001, // 0:LightGray 1:DarkGray 2:Black 3:White
};

const u8 obj_pal1[] = {
    0b00110101, // 0:LightGray 1:LightGray 2:Black 3:White
    0b00110101, // 0:LightGray 1:LightGray 2:Black 3:White
    0b00110101, // 0:LightGray 1:LightGray 2:Black 3:White
    0b00110101, // 0:LightGray 1:LightGray 2:Black 3:White
    0b00110101, // 0:LightGray 1:LightGray 2:Black 3:White
    0b01110101, // 0:LightGray 1:LightGray 2:Black 3:LightGray
    0b10110101, // 0:LightGray 1:LightGray 2:Black 3:DarkGray
    0b01110101, // 0:LightGray 1:LightGray 2:Black 3:LightGray
};

#define CBLACK  RGB8(0x00, 0x00, 0x00)
#define CDGRAY  RGB8(0x5f, 0x57, 0x4f)
#define CLGRAY  RGB8(0xc2, 0xc3, 0xc7)
#define CWHITE  RGB8(0xff, 0xf1, 0xe8)
#define CYELLOW RGB8(0xff, 0xec, 0x27)
#define CORANGE RGB8(0xff, 0xa3, 0x00)
#define CRED    RGB8(0xff, 0x00, 0x4d)
#define CPINK   RGB8(0xff, 0x77, 0xa8)

const palette_color_t cgb_bkg_pals[] = {
  CLGRAY,  CDGRAY,  CBLACK, CYELLOW,  // bg0: ground
  CWHITE,  CDGRAY,  CBLACK, CWHITE,   // bg1: saws/text
  CDGRAY,  CDGRAY,  CBLACK, CDGRAY,   // bg2: game over stats text
  CLGRAY,  CDGRAY,  CBLACK, CLGRAY,   // bg3: title credit text
  CLGRAY,  CDGRAY,  CBLACK, CDGRAY,   // bg4: title credit text
};

const palette_color_t cgb_obp_pals[] = {
  CYELLOW, CYELLOW, CBLACK, CYELLOW,  // spr0: mobs/pickups/interact.
  CORANGE, CORANGE, CBLACK, CORANGE,  // spr1: mobs damage float
  CRED,    CRED,    CRED,   CBLACK,   // spr2: player damage float
  CWHITE,  CWHITE,  CBLACK, CWHITE,   // spr3: mob hit flash
  CYELLOW, CYELLOW, CBLACK, CYELLOW,  // spr4: mob key flash
  CLGRAY,  CLGRAY,  CBLACK, CLGRAY,   // spr5: text/messages
  CLGRAY,  CDGRAY,  CBLACK, CYELLOW,  // spr6: PRESS START
};


const palette_color_t cgb_player_dmg_flash_pals[] = {
  CRED, CRED,   CRED, CBLACK,
  CRED, CPINK,  CRED, CBLACK,
  CRED, CWHITE, CRED, CBLACK,
  CRED, CRED,   CRED, CBLACK,
  CRED, CRED,   CRED, CBLACK,
  CRED, CPINK,  CRED, CBLACK,
  CRED, CWHITE, CRED, CBLACK,
  CRED, CRED,   CRED, CBLACK,
};

const palette_color_t cgb_mob_key_flash_pals[] = {
  CYELLOW, CYELLOW, CBLACK, CYELLOW,
  CYELLOW, CYELLOW, CBLACK, CYELLOW,
  CYELLOW, CYELLOW, CBLACK, CYELLOW,
  CYELLOW, CYELLOW, CBLACK, CYELLOW,
  CYELLOW, CYELLOW, CBLACK, CYELLOW,
  CYELLOW, CYELLOW, CBLACK, CYELLOW,
  CYELLOW, CYELLOW, CBLACK, CWHITE,
  CYELLOW, CYELLOW, CBLACK, CWHITE,
};

const palette_color_t cgb_press_start_flash_pals[] = {
  CLGRAY,  CBLACK,  CBLACK, CDGRAY,
  CLGRAY,  CBLACK,  CBLACK, CDGRAY,
  CLGRAY,  CBLACK,  CBLACK, CLGRAY,
  CLGRAY,  CBLACK,  CBLACK, CLGRAY,
  CLGRAY,  CBLACK,  CBLACK, CLGRAY,
  CLGRAY,  CBLACK,  CBLACK, CWHITE,
  CLGRAY,  CBLACK,  CBLACK, CWHITE,
  CLGRAY,  CBLACK,  CBLACK, CWHITE,
};

const Dir invdir[] = {DIR_RIGHT, DIR_LEFT, DIR_DOWN, DIR_UP}; // L R U D

const u8 dirx[] = {0xff, 1, 0, 0};  // L R U D
const u8 diry[] = {0, 0, 0xff, 1};  // L R U D
const u8 dirx2[] = {0xfe, 2, 0, 0}; // L R U D
const u8 diry2[] = {0, 0, 0xfe, 2}; // L R U D
const u8 dirx3[] = {0xfd, 3, 0, 0}; // L R U D
const u8 diry3[] = {0, 0, 0xfd, 3}; // L R U D
const u8 dirx4[] = {0xfc, 4, 0, 0}; // L R U D
const u8 diry4[] = {0, 0, 0xfc, 4}; // L R U D
const u8 n_over_3[] = {0,  3,  6,  8,  11, 14, 16, 19,
                       22, 24, 27, 30, 32, 35, 38, 40};
const u16 three_over_n[] = {768, 384, 256, 192, 154, 128, 110, 96,
                            85,  77,  70,  64,  59,  55,  51};

// Movement is reversed since it is accessed backward. Stored as differences
// from the last position instead of absolute offset.
const u8 hopy4[] = {1, 1, 1, 1, 0xff, 0xff, 0xff, 0xff};
const u8 hopy12[] = {4, 4, 3, 1, 0xff, 0xfd, 0xfc, 0xfc};
const u8 pickbounce[] = {1, 0, 0, 0xff, 0xff, 0, 0, 1};

const u8 sightdiff[] = {
    0x00,
    0xff, 0xee, 0xfe, 0xed, 0xef, 0xde,
    0x0f, 0xfe, 0xfd, 0x0e, 0x0d, 0x0c, 0x1e, 0x1d,
    0x1f, 0x1e, 0x2d, 0x2e, 0x2f, 0x3e,
    0xf0, 0xef, 0xdf, 0xe0, 0xd0, 0xc0, 0xe1, 0xd1,
    0x10, 0x2f, 0x3f, 0x20, 0x30, 0x40, 0x21, 0x31,
    0xf1, 0xe1, 0xd2, 0xe2, 0xf2, 0xe3,
    0x01, 0xf2, 0xf3, 0x02, 0x03, 0x04, 0x12, 0x13,
    0x11, 0x21, 0x32, 0x12, 0x23, 0x22,
};

const u8 sightdiffblind[] = {
    0x00, 0x0f, 0xf0, 0x10, 0x01,
};

const u8 sightskip[] = {
    0,
    5, 0, 1, 0, 1, 0,
    7, 1, 0, 2, 1, 0, 1, 0,
    5, 1, 0, 0, 1, 0,
    7, 1, 0, 2, 1, 0, 1, 0,
    7, 1, 0, 2, 1, 0, 1, 0,
    5, 1, 0, 0, 1, 0,
    7, 1, 0, 2, 1, 0, 1, 0,
    5, 1, 0, 1, 0, 0,
};

const u8 dirpos[] = {0xff, 1,    0xf0, 16,
                     0xef, 0xf1, 15,   17}; // L R U D UL UR DL DR

const u8 dirvalid[] = {VALID_L,  VALID_R,  VALID_U,  VALID_D,
                       VALID_UL, VALID_UR, VALID_DL, VALID_DR};

const u8 mob_type_hp[] = {
    5,  // player
    1,  // slime
    1,  // queen
    1,  // scorpion
    2,  // hulk
    1,  // ghost
    1,  // kong
    10, // reaper
    1,  // weed
    1,  // bomb
    1,  // vase1
    1,  // vase2
    1,  // chest
    1,  // heart chest
    1,  // total
};

const u8 mob_type_anim_frames[] = {
    0x99, 0x9a, 0x9b, 0x9c,                   // player
    0x80, 0x81, 0x80, 0x82,                   // slime
    0x86, 0x87, 0x88, 0x89,                   // queen
    0x8a, 0x8b, 0x8a, 0x8c,                   // scorpion
    0x83, 0x84, 0x83, 0x85,                   // hulk
    0x8d, 0x8e, 0x8f, 0x8e,                   // ghost
    0x90, 0x91, 0x92, 0x93,                   // kong
    0x9d, 0x9e, 0x9f, 0xa0,                   // reaper
    0x94, 0x95, 0x96, 0x95,                   // weed
    0x97, 0x97, 0x97, 0x97, 0x97, 0x97, 0x97, //
    0x97, 0x97, 0x97, 0x97, 0x97, 0x98,       // bomb
    0x4a,                                     // vase1
    0x4b,                                     // vase2
    0x4c,                                     // chest
    0x4c,                                     // heart chest
};

const u8 mob_type_anim_speed[] = {
    24, // player
    12, // slime
    12, // queen
    12, // scorpion
    13, // hulk
    16, // ghost
    12, // kong
    16, // reaper
    16, // weed
    8,  // bomb
    255, // vase1
    255, // vase2
    255, // chest
    255, // heart chest
};

const u8 mob_type_anim_start[] = {
    0,  // player
    4,  // slime
    8,  // queen
    12, // scorpion
    16, // hulk
    20, // ghost
    24, // kong
    28, // reaper
    32, // weed
    36, // bomb
    49, // vase1
    50, // vase2
    51, // chest
    52, // heart chest
    53, // total
};

const MobAI mob_type_ai_wait[] = {
    MOB_AI_NONE,   // player
    MOB_AI_WAIT,   // slime
    MOB_AI_WAIT,   // queen
    MOB_AI_WAIT,   // scorpion
    MOB_AI_WAIT,   // hulk
    MOB_AI_WAIT,   // ghost
    MOB_AI_WAIT,   // kong
    MOB_AI_REAPER, // reaper
    MOB_AI_WEED,   // weed
    MOB_AI_NONE,   // bomb
    MOB_AI_NONE,   // vase1
    MOB_AI_NONE,   // vase2
    MOB_AI_NONE,   // chest
    MOB_AI_NONE,   // heart chest
};

const MobAI mob_type_ai_active[] = {
    MOB_AI_NONE,   // player
    MOB_AI_ATTACK, // slime
    MOB_AI_QUEEN,  // queen
    MOB_AI_ATTACK, // scorpion
    MOB_AI_ATTACK, // hulk
    MOB_AI_ATTACK, // ghost
    MOB_AI_KONG,   // kong
    MOB_AI_REAPER, // reaper
    MOB_AI_WEED,   // weed
    MOB_AI_NONE,   // bomb
    MOB_AI_NONE,   // vase1
    MOB_AI_NONE,   // vase2
    MOB_AI_NONE,   // chest
    MOB_AI_NONE,   // heart chest
};

const u8 mob_type_object[] = {
    0, // player
    0, // slime
    0, // queen
    0, // scorpion
    0, // hulk
    0, // ghost
    0, // kong
    0, // reaper
    0, // weed
    1, // bomb
    1, // vase1
    1, // vase2
    1, // chest
    1, // heart chest
};

const u8 dirt_tiles[] = {TILE_EMPTY, TILE_DIRT1, TILE_DIRT2, TILE_DIRT3};

const u8 pick_type_anim_frames[] = {
    0xc2, 0xc3, 0xc2, 0xc4, 0xc2, 0xc2, 0xc2, 0xc2, // heart
    0xc5, 0xc6, 0xc5, 0xc7, 0xc5, 0xc5, 0xc5, 0xc5, // key
    0xc8, 0xc9, 0xca, 0xc9, 0xc8, 0xc8, 0xc8, 0xc8, // pickup ring
};

const u8 pick_type_anim_start[] = {
    0,  // PICKUP_TYPE_HEART,
    8,  // PICKUP_TYPE_KEY,
    16, // PICKUP_TYPE_JUMP,
    16, // PICKUP_TYPE_BOLT,
    16, // PICKUP_TYPE_PUSH,
    16, // PICKUP_TYPE_GRAPPLE,
    16, // PICKUP_TYPE_SPEAR,
    16, // PICKUP_TYPE_SMASH,
    16, // PICKUP_TYPE_HOOK,
    16, // PICKUP_TYPE_SPIN,
    16, // PICKUP_TYPE_SUPLEX,
    16, // PICKUP_TYPE_SLAP,
};

const u8 pick_type_sprite_tile[] = {
    0,    // PICKUP_TYPE_HEART,
    0,    // PICKUP_TYPE_KEY,
    0xf2, // PICKUP_TYPE_JUMP,
    0xf3, // PICKUP_TYPE_BOLT,
    0xf4, // PICKUP_TYPE_PUSH,
    0xf5, // PICKUP_TYPE_GRAPPLE,
    0xf6, // PICKUP_TYPE_SPEAR,
    0xf7, // PICKUP_TYPE_SMASH,
    0xf8, // PICKUP_TYPE_HOOK,
    0xf9, // PICKUP_TYPE_SPIN,
    0xfa, // PICKUP_TYPE_SUPLEX,
    0xfb, // PICKUP_TYPE_SLAP,
};

const u8 pick_type_name_tile[] = {
    212, 223, 215, 218, 0,   239, 231, 240,                // JUMP (2)
    204, 217, 214, 222, 0,   239, 231, 240,                // BOLT (2)
    218, 223, 221, 210, 0,   239, 231, 240,                // PUSH (2)
    209, 220, 203, 218, 218, 214, 207, 0,   239, 231, 240, // GRAPPLE (2)
    221, 218, 207, 203, 220, 0,   239, 231, 240,           // SPEAR (2)
    221, 215, 203, 221, 210, 0,   239, 231, 240,           // SMASH (2)
    210, 217, 217, 213, 0,   239, 231, 240,                // HOOK (2)
    221, 218, 211, 216, 0,   239, 231, 240,                // SPIN (2)
    221, 223, 218, 214, 207, 226, 0,   239, 231, 240,      // SUPLEX (2)
    221, 214, 203, 218, 0,   239, 231, 240,                // SLAP (2)
};

const u8 pick_type_name_start[] = {0,  0,  0,  8,  16, 24, 35,
                                   44, 53, 61, 69, 79, 87};

const u8 pick_type_name_len[] = {0, 0, 8, 8, 8, 11, 9, 9, 8, 8, 10, 8};

const u8 pick_type_desc_tile[] = {
    // JUMP 2 SPACES
    212, 223, 215, 218,   0, 231,   0, 221, 218, 203, 205, 207, 221,
    // STOP ENEMIES
    221, 222, 217, 218,   0, 207, 216, 207, 215, 211, 207, 221,
    // YOU SKIP OVER
    227, 217, 223,   0, 221, 213, 211, 218,   0, 217, 224, 207, 220,
    // RANGED ATTACK
    220, 203, 216, 209, 207, 206,   0, 203, 222, 222, 203, 205, 213,
    // DO 1 DAMAGE
    206, 217,   0, 230,   0, 206, 203, 215, 203, 209, 207,
    // STOP ENEMY
    221, 222, 217, 218,   0, 207, 216, 207, 215, 227,
    // RANGED ATTACK
    220, 203, 216, 209, 207, 206,   0, 203, 222, 222, 203, 205, 213,
    // PUSH ENEMY
    218, 223, 221, 210,   0, 207, 216, 207, 215, 227,
    // 1 SPACE AND
    230,   0, 221, 218, 203, 205, 207,   0, 203, 216, 206,
    // STOP THEM
    221, 222, 217, 218,   0, 222, 210, 207, 215,
    // PULL YOURSELF
    218, 223, 214, 214,   0, 227, 217, 223, 220, 221, 207, 214, 208,
    // UP TO THE NEXT
    223, 218,   0, 222, 217,   0, 222, 210, 207,   0, 216, 207, 226, 222,
    // OCCUPIED SPACE
    217, 205, 205, 223, 218, 211, 207, 206,   0, 221, 218, 203, 205, 207,
    // HIT 2 SPACES
    210, 211, 222,   0, 231,   0, 221, 218, 203, 205, 207, 221,
    // IN ANY
    211, 216,   0, 203, 216, 227,
    // DIRECTION
    206, 211, 220, 207, 205, 222, 211, 217, 216,
    // SMASH A WALL
    221, 215, 203, 221, 210,   0, 203,   0, 225, 203, 214, 214,
    // OR DO 2
    217, 220,   0, 206, 217,   0, 231,
    // DAMAGE
    206, 203, 215, 203, 209, 207,
    // PULL AN ENEMY
    218, 223, 214, 214,   0, 203, 216,   0, 207, 216, 207, 215, 227,
    // 1 SPACE TOWARD
    230,   0, 221, 218, 203, 205, 207,   0, 222, 217, 225, 203, 220, 206,
    // YOU AND STOP
    227, 217, 223,   0, 203, 216, 206,   0, 221, 222, 217, 218,
    // THEM
    222, 210, 207, 215,
    // HIT 4 SPACES
    210, 211, 222,   0, 233,   0, 221, 218, 203, 205, 207, 221,
    // AROUND YOU
    203, 220, 217, 223, 216, 206,   0, 227, 217, 223,
    // LIFT AND THROW
    214, 211, 208, 222,   0, 203, 216, 206,   0, 222, 210, 220, 217, 225,
    // AN ENEMY
    203, 216,   0, 207, 216, 207, 215, 227,
    // BEHIND YOU AND
    204, 207, 210, 211, 216, 206,   0, 227, 217, 223,   0, 203, 216, 206,
    // STOP THEM
    221, 222, 217, 218,   0, 222, 210, 207, 215,
    // HIT AN ENEMY
    210, 211, 222,   0, 203, 216,   0, 207, 216, 207, 215, 227,
    // AND HOP
    203, 216, 206,   0, 210, 217, 218,
    // BACKWARD
    204, 203, 205, 213, 225, 203, 220, 206,
    // STOP ENEMY
    221, 222, 217, 218,   0, 207, 216, 207, 215, 227
};

const u16 pick_type_desc_start[][NUM_INV_ROWS] = {
    {  0,   0,   0,   0}, // PICKUP_TYPE_HEART,
    {  0,   0,   0,   0}, // PICKUP_TYPE_KEY,
    {  0,  13,  25,   0}, // PICKUP_TYPE_JUMP,
    { 38,  51,  62,   0}, // PICKUP_TYPE_BOLT,
    { 72,  85,  95, 106}, // PICKUP_TYPE_PUSH,
    {115, 128, 142,   0}, // PICKUP_TYPE_GRAPPLE,
    {156, 168, 174,   0}, // PICKUP_TYPE_SPEAR,
    {183, 195, 202,   0}, // PICKUP_TYPE_SMASH,
    {208, 221, 235, 247}, // PICKUP_TYPE_HOOK,
    {251, 263,   0,   0}, // PICKUP_TYPE_SPIN,
    {273, 287, 295, 309}, // PICKUP_TYPE_SUPLEX,
    {318, 330, 337, 345}, // PICKUP_TYPE_SLAP,
};

const u8 pick_type_desc_len[][NUM_INV_ROWS] = {
    { 0,  0,  0,  0}, // PICKUP_TYPE_HEART,
    { 0,  0,  0,  0}, // PICKUP_TYPE_KEY,
    {13, 12, 13,  0}, // PICKUP_TYPE_JUMP,
    {13, 11, 10,  0}, // PICKUP_TYPE_BOLT,
    {13, 10, 11,  9}, // PICKUP_TYPE_PUSH,
    {13, 14, 14,  0}, // PICKUP_TYPE_GRAPPLE,
    {12,  6,  9,  0}, // PICKUP_TYPE_SPEAR,
    {12,  7,  6,  0}, // PICKUP_TYPE_SMASH,
    {13, 14, 12,  4}, // PICKUP_TYPE_HOOK,
    {12, 10,  0,  0}, // PICKUP_TYPE_SPIN,
    {14,  8, 14,  9}, // PICKUP_TYPE_SUPLEX,
    {12,  7,  8, 10}, // PICKUP_TYPE_SLAP,
};

const u8 float_diff_y[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
                           1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1};

const u8 float_dmg[] = {0, 0x14, 0x15, 0x16};

const u8 float_pick_type_tiles[] = {
    0x17,                   // PICKUP_TYPE_HEART,
    0x32, 0x33,             // PICKUP_TYPE_KEY,
    0x18, 0x19, 0x1a,       // PICKUP_TYPE_JUMP,
    0x1b, 0x1c, 0x1d,       // PICKUP_TYPE_BOLT,
    0x1e, 0x1f, 0x20,       // PICKUP_TYPE_PUSH,
    0x21, 0x22, 0x23, 0x24, // PICKUP_TYPE_GRAPPLE,
    0x25, 0x26, 0x27,       // PICKUP_TYPE_SPEAR,
    0x28, 0x29, 0x2a,       // PICKUP_TYPE_SMASH,
    0x2b, 0x2c, 0x20,       // PICKUP_TYPE_HOOK,
    0x25, 0x2d, 0x2e,       // PICKUP_TYPE_SPIN,
    0x2f, 0x23, 0x30, 0x20, // PICKUP_TYPE_SUPLEX,
    0x31, 0x22, 0x1a,       // PICKUP_TYPE_SLAP,
    0x34, 0x35, 0x36,       // PICKUP_TYPE_FULL
};

const u8 float_pick_type_start[] = {
    0,  // PICKUP_TYPE_HEART,
    1,  // PICKUP_TYPE_KEY,
    3,  // PICKUP_TYPE_JUMP,
    6,  // PICKUP_TYPE_BOLT,
    9,  // PICKUP_TYPE_PUSH,
    12, // PICKUP_TYPE_GRAPPLE,
    16, // PICKUP_TYPE_SPEAR,
    19, // PICKUP_TYPE_SMASH,
    22, // PICKUP_TYPE_HOOK,
    25, // PICKUP_TYPE_SPIN,
    28, // PICKUP_TYPE_SUPLEX,
    32, // PICKUP_TYPE_SLAP,
    35, // PICKUP_TYPE_FULL
    38, // total
};

const u8 float_pick_type_x_offset[] = {
    0,    // PICKUP_TYPE_HEART,
    0xfe, // PICKUP_TYPE_KEY,
    0xfc, // PICKUP_TYPE_JUMP,
    0xfc, // PICKUP_TYPE_BOLT,
    0xfc, // PICKUP_TYPE_PUSH,
    0xf6, // PICKUP_TYPE_GRAPPLE,
    0xfa, // PICKUP_TYPE_SPEAR,
    0xfa, // PICKUP_TYPE_SMASH,
    0xfc, // PICKUP_TYPE_HOOK,
    0xfc, // PICKUP_TYPE_SPIN,
    0xf8, // PICKUP_TYPE_SUPLEX,
    0xfc, // PICKUP_TYPE_SLAP,
    0xfb, // PICKUP_TYPE_FULL
};

// Drop position offsets checked by distance, up to -8 + 7 in both directions
const u8 drop_diff[] = {
    0x00, 0xff, 0xf0, 0x01, 0x10, 0x0f, 0xfe, 0xef, 0xe0, 0xf1, 0x02, 0x11,
    0x20, 0x21, 0x12, 0x03, 0xf2, 0xe1, 0xd0, 0xdf, 0xee, 0xfd, 0x0e, 0x1f,
    0x30, 0x40, 0x2f, 0x1e, 0x0d, 0xfc, 0xed, 0xde, 0xcf, 0xc0, 0xd1, 0xe2,
    0xf3, 0x04, 0x13, 0x22, 0x31, 0x41, 0x50, 0x3f, 0x2e, 0x1d, 0x0c, 0xfb,
    0xec, 0xdd, 0xce, 0xbf, 0xb0, 0xc1, 0xd2, 0xe3, 0xf4, 0x05, 0x14, 0x23,
    0x32, 0x33, 0x24, 0x15, 0x06, 0xf5, 0xe4, 0xd3, 0xc2, 0xb1, 0xa0, 0xaf,
    0xbe, 0xcd, 0xdc, 0xeb, 0xfa, 0x0b, 0x1c, 0x2d, 0x3e, 0x4f, 0x60, 0x51,
    0x42, 0x43, 0x34, 0x25, 0x16, 0x07, 0xf6, 0xe5, 0xd4, 0xc3, 0xb2, 0xa1,
    0x90, 0x9f, 0xae, 0xbd, 0xcc, 0xdb, 0xea, 0xf9, 0x0a, 0x1b, 0x2c, 0x3d,
    0x4e, 0x5f, 0x70, 0x61, 0x52, 0x53, 0x44, 0x35, 0x26, 0x17, 0xf7, 0xe6,
    0xd5, 0xc4, 0xb3, 0xa2, 0x91, 0x80, 0x8f, 0x9e, 0xad, 0xbc, 0xcb, 0xda,
    0xe9, 0xf8, 0x09, 0x1a, 0x2b, 0x3c, 0x4d, 0x5e, 0x6f, 0x71, 0x62, 0x63,
    0x54, 0x45, 0x36, 0x27, 0xe7, 0xd6, 0xc5, 0xb4, 0xa3, 0x92, 0x81, 0x7f,
    0x8e, 0x9d, 0xac, 0xbb, 0xca, 0xd9, 0xe8, 0x08, 0x19, 0x2a, 0x3b, 0x4c,
    0x5d, 0x6e, 0x72, 0x73, 0x64, 0x55, 0x46, 0x37, 0xd7, 0xc6, 0xb5, 0xa4,
    0x93, 0x82, 0x7e, 0x8d, 0x9c, 0xab, 0xba, 0xc9, 0xd8, 0x18, 0x29, 0x3a,
    0x4b, 0x5c, 0x6d, 0x6c, 0x5b, 0x4a, 0x39, 0x28, 0xc8, 0xb9, 0xaa, 0x9b,
    0x8c, 0x7d, 0x83, 0x94, 0xa5, 0xb6, 0xc7, 0x47, 0x56, 0x65, 0x74, 0x75,
    0x66, 0x57, 0xb7, 0xa6, 0x95, 0x84, 0x7c, 0x8b, 0x9a, 0xa9, 0xb8, 0x38,
    0x49, 0x5a, 0x6b, 0x6a, 0x59, 0x48, 0xa8, 0x99, 0x8a, 0x7b, 0x85, 0x96,
    0xa7, 0x67, 0x76, 0x77, 0x69, 0x58, 0x98, 0x89, 0x7a, 0x86, 0x97, 0x87,
    0x79, 0x88, 0x68, 0x78,
};

const u8 inventory_map[] = {
    112, 113, 113, 114, 113, 113, 114, 113, 113, 113, 113, 113, 113, 113, 113, 115,
    116, 228, 234, 117, 197, 229, 117, 208, 214, 217, 217, 220, 0,   0,   0,   118,
    119, 120, 120, 121, 120, 120, 121, 120, 120, 120, 120, 120, 120, 120, 120, 122,
    116, 0,   0,   219, 219, 219, 0,   0,   0,   0,   0,   0,   0,   0,   0,   118,
    116, 0,   0,   219, 219, 219, 0,   0,   0,   0,   0,   0,   0,   0,   0,   118,
    116, 0,   0,   219, 219, 219, 0,   0,   0,   0,   0,   0,   0,   0,   0,   118,
    116, 0,   0,   219, 219, 219, 0,   0,   0,   0,   0,   0,   0,   0,   0,   118,
    119, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 122,
    116, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   118,
    116, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   118,
    116, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   118,
    116, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   118,
    123, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 125};

const u8 blind_map[] = {204, 214, 211, 216, 206};

const u8 inventory_up_y[] = {40, 41, 41, 41, 41, 42,  42, 43, 43,
                             44, 45, 46, 48, 49, 51,  54, 57, 60,
                             64, 69, 75, 82, 90, 101, 113};

const u8 inventory_down_y[] = {128, 127, 127, 127, 127, 126, 126, 125, 125,
                               124, 123, 122, 120, 119, 117, 114, 111, 108,
                               104, 99,  93,  86,  78,  67,  55};

const u8 msg_tiles[] = {
  // "wurstlord awakens"
  0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63,
  0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64,
  // "one monster holds the key"
  0x65, 0x66, 0x67, 0x68, 0x5d, 0x69, 0x6a, 0x6b, 0x63,
  0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x6d, 0x6d, 0x72,
  // "wurstchain"
  0x5b, 0x5c, 0x5d, 0x76, 0x77, 0x78,
  0x6c, 0x6d, 0x6d, 0x6d, 0x6d, 0x72,   0x6d, // <- number
};

// Explosion sprites
const u8 boom_spr_speed[NUM_BOOM_SPRS] = {40, 19, 20, 38, 41, 39, 38, 22,
                                          39, 31, 15, 34, 22, 26, 36, 45};
const u8 boom_spr_anim_speed[NUM_BOOM_SPRS] = {10, 5, 5, 10, 11, 10, 10, 6,
                                               10, 8, 4, 9,  6,  7,  9,  12};
const u16 boom_spr_x[NUM_BOOM_SPRS] = {64665, 757,   65447, 65176, 841,  64448,
                                       65167, 398,   49,    570,   1221, 65345,
                                       807,   65415, 303,   351};
const u16 boom_spr_y[NUM_BOOM_SPRS] = {125,   65459, 65415, 541,   64906, 65047,
                                       270,   277,   65264, 65337, 65419, 65041,
                                       65281, 785,   479,   65493};
const u16 boom_spr_dx[NUM_BOOM_SPRS] = {65301, 177,   65459, 65409, 196, 65394,
                                        65378, 202,   39,    166,   245, 65483,
                                        243,   65498, 114,   135};
const u16 boom_spr_dy[NUM_BOOM_SPRS] = {
    33,    65518, 65432, 190,   65389, 65472, 115, 140,
    65318, 65477, 65512, 65400, 65459, 243,   180, 65519};

const u8 dead_map[] = {
    0,   0,   1,   2, 3,   4,   0,   0,
    0,   0,   5,   6, 7,   8,   0,   0,
    0,   0,   0,   9, 10,  0,   0,   0,
    0,   0,   0,   0, 0,   0,   0,   0,
    227, 217, 223, 0, 206, 211, 207, 206
};

const u8 win_map[] = {
    0,   0,   0,   0,   11,  12,  13,  14,  0,   0,   0,   0,
    0,   0,   0,   0,   15,  16,  17,  18,  0,   0,   0,   0,
    0,   0,   0,   0,   19,  20,  21,  22,  0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    222, 210, 207, 0,   213, 211, 207, 214, 204, 203, 221, 203,
    0,   0,   211, 221, 0,   227, 217, 223, 220, 221, 0,   0
};

const u8 stats_map[] = {
    208, 214, 217, 217, 220, 241, 0,   0,   0,
    221, 222, 207, 218, 221, 241, 0,   0,   0,
    213, 211, 214, 214, 221, 241, 0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   218, 220, 207, 221, 221, 0,   252, 0,
};

#include <gb/gb.h>
#include <gb/gbdecompress.h>
#include <rand.h>
#include <string.h>

#include "tilebg.c"
#include "tileshared.c"
#include "tilesprites.c"
#include "map.c"
#include "flags.c"

typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef void (*vfp)(void);

#define MAX_DS_SET 64 /* XXX can be smaller */
#define MAX_ROOMS 4
#define MAX_VOIDS 10 /* XXX Figure out what this should be */
#define MAX_CANDS 256
#define MAX_DEAD_MOBS 4
#define MAX_MOBS 32    /* XXX Figure out what this should be */
#define MAX_PICKUPS 16 /* XXX Figure out what this should be */
#define MAX_FLOATS 8   /* For now; we'll probably want more */
#define MAX_EQUIPS 4
#define MAX_SPRS 16 /* Animation sprites for explosions, etc. */
#define MAX_SAWS 32 /* XXX */

#define BASE_FLOAT 18

#define MSG_REAPER 0
#define MSG_REAPER_Y 83
#define MSG_KEY 18
#define MSG_KEY_Y 80
#define MSG_LEN 18

// TODO: how big to make these arrays?
#define TILE_CODE_SIZE 128
#define MOB_TILE_CODE_SIZE 256

#define PLAYER_MOB 0

#define NUM_GATE_MAPS 14

#define MAP_WIDTH 16
#define MAP_HEIGHT 16
#define INV_WIDTH 16
#define INV_HEIGHT 13
#define INV_HP_ADDR 0x9c22
#define INV_KEYS_ADDR 0x9c25
#define INV_FLOOR_ADDR 0x9c2d
#define INV_EQUIP_ADDR 0x9c63
#define INV_DESC_ADDR 0x9d01
#define INV_SELECT_X_OFFSET 32
#define INV_SELECT_Y_OFFSET 40
#define INV_ROW_LEN 14
#define NUM_INV_ROWS 4

#define TILE_ANIM_FRAMES 8
#define TILE_ANIM_FRAME_DIFF 16
#define TILE_FLIP_DIFF 0x21
#define TILE_MOB_FLASH_DIFF 0x46
#define TILE_VOID_EXIT_DIFF 0x35
#define TILE_VOID_OPEN_DIFF 3

#define FADE_FRAMES 8
#define INV_ANIM_FRAMES 43

#define WALK_FRAMES 8
#define BUMP_FRAMES 4
#define MOB_HOP_FRAMES 8
#define PICK_HOP_FRAMES 8
#define FLOAT_FRAMES 70
#define OBJ1_PAL_FRAMES 8
#define MOB_FLASH_FRAMES 20
#define DEAD_MOB_FRAMES 10
#define MSG_FRAMES 120
#define INV_SELECT_FRAMES 8

#define QUEEN_CHARGE_MOVES 2
#define AI_COOL_MOVES 8
#define RECOVER_MOVES 30

#define REAPER_AWAKENS_STEPS 100

#define SIGHT_DIST_BLIND 1
#define SIGHT_DIST_DEFAULT 4

#define FLOAT_BLIND_X_OFFSET 0xfb

#define PICKUP_FRAMES 6
#define PICKUP_CHARGES 2

#define IS_WALL_TILE(tile)             (flags_bin[tile] & 0b00000001)
#define IS_SPECIAL_TILE(tile)          (flags_bin[tile] & 0b00000010)
#define IS_WALL_OR_SPECIAL_TILE(tile)  (flags_bin[tile] & 0b00000011)
#define IS_OPAQUE_TILE(tile)           (flags_bin[tile] & 0b00000100)
#define IS_CRACKED_WALL_TILE(tile)     (flags_bin[tile] & 0b00001000)
#define IS_ANIMATED_TILE(tile)         (flags_bin[tile] & 0b00010000)
#define IS_WALL_FACE_TILE(tile)        (flags_bin[tile] & 0b00100000)
#define TILE_HAS_CRACKED_VARIANT(tile) (flags_bin[tile] & 0b01000000)

#define IS_DOOR(pos) sigmatch((pos), doorsig, doormask)
#define CAN_CARVE(pos) sigmatch((pos), carvesig, carvemask)
#define IS_FREESTANDING(pos) sigmatch((pos), freesig, freemask)

#define IS_MOB(pos) (mobmap[pos])
#define IS_SMARTMOB(tile, pos) ((flags_bin[tile] & 3) || mobmap[pos])
#define IS_MOB_AI(tile, pos)                                                   \
  ((flags_bin[tile] & 3) || (mobmap[pos] && !mob_active[mobmap[pos] - 1]))
#define IS_UNSPECIAL_WALL_TILE(tile)                                           \
  ((flags_bin[tile] & 0b00000011) == 0b00000001)

#define URAND() ((u8)rand())
#define URAND_10_PERCENT() (URAND() < 26)
#define URAND_12_5_PERCENT() ((URAND() & 7) == 0)
#define URAND_20_PERCENT() (URAND() < 51)
#define URAND_25_PERCENT() ((URAND() & 3) == 0)
#define URAND_33_PERCENT() (URAND() < 85)
#define URAND_50_PERCENT() (URAND() & 1)

#define MAP_X_OFFSET 2
#define MAP_Y_OFFSET 0
#define POS_TO_ADDR(pos) (0x9802 + (((pos)&0xf0) << 1) + ((pos)&0xf))
#define POS_TO_X(pos) (((pos & 0xf) << 3) + 24)
#define POS_TO_Y(pos) (((pos & 0xf0) >> 1) + 16)

#define VALID_UL 0b00000001
#define VALID_DL 0b00000010
#define VALID_DR 0b00000100
#define VALID_UR 0b00001000
#define VALID_D  0b00010000
#define VALID_U  0b00100000
#define VALID_R  0b01000000
#define VALID_L  0b10000000
#define VALID_L_OR_UL  0b10000001
#define VALID_U_OR_UL  0b00100001

#define MASK_UL  0b11111110
#define MASK_DL  0b11111101
#define MASK_DR  0b11111011
#define MASK_UR  0b11110111
#define MASK_D   0b11101111
#define MASK_U   0b11011111
#define MASK_R   0b10111111
#define MASK_L   0b01111111

#define POS_UL(pos)       ((u8)(pos + 239))
#define POS_U(pos)        ((u8)(pos + 240))
#define POS_UR(pos)       ((u8)(pos + 241))
#define POS_L(pos)        ((u8)(pos + 255))
#define POS_R(pos)        ((u8)(pos + 1))
#define POS_DL(pos)       ((u8)(pos + 15))
#define POS_D(pos)        ((u8)(pos + 16))
#define POS_DR(pos)       ((u8)(pos + 17))
#define POS_DIR(pos, dir) ((u8)(pos + dirpos[dir]))

// BG tiles
#define TILE_NONE 0
#define TILE_EMPTY 1
#define TILE_WALL 2
#define TILE_WALL_FACE_CRACKED 3
#define TILE_WALL_FACE 4
#define TILE_WALL_FACE_RUBBLE 5
#define TILE_WALL_FACE_PLANT 6
#define TILE_TELEPORTER 7
#define TILE_CARPET 8
#define TILE_CHEST_OPEN 9
#define TILE_VOID1 0xa
#define TILE_VOID2 0xb
#define TILE_END 0xe
#define TILE_START 0xf
#define TILE_GATE 0x3f
#define TILE_TORCH_LEFT 0x40
#define TILE_TORCH_RIGHT 0x41
#define TILE_VOID_BUTTON_U 0x46
#define TILE_VOID_BUTTON_L 0x55
#define TILE_VOID_BUTTON_R 0x57
#define TILE_VOID_BUTTON_D 0x66
#define TILE_HEART 0x48
#define TILE_RUG 0x59
#define TILE_BOMB_EXPLODED 0x5b
#define TILE_DIRT1 0x6a
#define TILE_DIRT2 0x6b
#define TILE_DIRT3 0x6c
#define TILE_PLANT1 0xc
#define TILE_PLANT2 0xd
#define TILE_PLANT3 0x5c
#define TILE_FIXED_WALL 0x47
#define TILE_SAW 0x42
#define TILE_SAW_BROKEN 0x53
#define TILE_STEPS 0x60
#define TILE_ENTRANCE 0x61

#define TILE_WALLSIG_BASE 0xf
#define TILE_CRACKED_DIFF 0x34

// Sprite tiles
#define TILE_BLIND 0x37
#define TILE_BOOM 0x05

// Shared tiles
#define TILE_0 0xe5
#define TILE_1 0xe6

#define FLOAT_FOUND 0xe
#define FLOAT_LOST 0xf

typedef u8 Map[MAP_WIDTH * MAP_HEIGHT];

typedef enum Dir {
  DIR_LEFT,
  DIR_RIGHT,
  DIR_UP,
  DIR_DOWN,
} Dir;

typedef enum Turn {
  TURN_PLAYER,
  TURN_PLAYER_WAIT,
  TURN_AI,
  TURN_AI_WAIT,
  TURN_WEEDS,
  TURN_WEEDS_WAIT,
} Turn;

typedef enum RoomKind {
  ROOM_KIND_VASE,
  ROOM_KIND_DIRT,
  ROOM_KIND_CARPET,
  ROOM_KIND_TORCH,
  ROOM_KIND_PLANT,
  NUM_ROOM_KINDS,
} RoomKind;

typedef enum MobType {
  MOB_TYPE_PLAYER,
  MOB_TYPE_SLIME,
  MOB_TYPE_QUEEN,
  MOB_TYPE_SCORPION,
  MOB_TYPE_HULK,
  MOB_TYPE_GHOST,
  MOB_TYPE_KONG,
  MOB_TYPE_REAPER,
  MOB_TYPE_WEED,
  MOB_TYPE_BOMB,
  MOB_TYPE_VASE1,
  MOB_TYPE_VASE2,
  MOB_TYPE_CHEST,
  MOB_TYPE_HEART_CHEST,
  NUM_MOB_TYPES,
} MobType;

typedef enum PickupType {
  PICKUP_TYPE_HEART,
  PICKUP_TYPE_KEY,
  PICKUP_TYPE_JUMP,
  PICKUP_TYPE_BOLT,
  PICKUP_TYPE_PUSH,
  PICKUP_TYPE_GRAPPLE,
  PICKUP_TYPE_SPEAR,
  PICKUP_TYPE_SMASH,
  PICKUP_TYPE_HOOK,
  PICKUP_TYPE_SPIN,
  PICKUP_TYPE_SUPLEX,
  PICKUP_TYPE_SLAP,
  PICKUP_TYPE_FULL, // Used to display "full!" message
  PICKUP_TYPE_NONE = 0,
} PickupType;

typedef enum SprType {
  SPR_TYPE_BOOM,
  SPR_TYPE_SPIN,
} SprType;

typedef enum MobAnimState {
  MOB_ANIM_STATE_NONE,
  MOB_ANIM_STATE_WALK,
  MOB_ANIM_STATE_BUMP1,
  MOB_ANIM_STATE_BUMP2,
  MOB_ANIM_STATE_HOP4,
} MobAnimState;

typedef enum MobAI {
  MOB_AI_NONE,
  MOB_AI_WAIT,
  MOB_AI_WEED,
  MOB_AI_REAPER,
  MOB_AI_ATTACK,
  MOB_AI_QUEEN,
  MOB_AI_KONG,
} MobAI;

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

const u8 dirx[] = {0xff, 1, 0, 0};       // L R U D
const u8 diry[] = {0, 0, 0xff, 1};       // L R U D
const u8 dirpos[] = {0xff, 1,    0xf0, 16,
                     0xef, 0xf1, 15,   17}; // L R U D UL UR DL DR
const u8 dirvalid[] = {VALID_L,  VALID_R,  VALID_U,  VALID_D,
                       VALID_UL, VALID_UR, VALID_DL, VALID_DR};

// Movement is reversed since it is accessed backward. Stored as differences
// from the last position instead of absolute offset.
const u8 hopy4[] = {1, 1, 1, 1, 0xff, 0xff, 0xff, 0xff};
const u8 hopy12[] = {4, 4, 3, 1, 0xff, 0xfd, 0xfc, 0xfc};
const u8 pickbounce[] = {1, 0, 0, 0xff, 0xff, 0, 0, 1};

const u8 carvesig[] = {255, 223, 127, 191, 239, 0};
const u8 carvemask[] = {0, 9, 3, 12, 6};
const u8 doorsig[] = {207, 63, 0};
const u8 doormask[] = {15, 15};

const u8 freesig[] = {8, 4, 2, 1, 22, 76, 41, 131, 171, 109, 94, 151, 0};
const u8 freemask[] = {8, 4, 2, 1, 6, 12, 9, 3, 10, 5, 10, 5};

const u8 wallsig[] = {
    251, 239, 253, 95,  159, 91,  31,  157, 115, 217, 241, 248,
    219, 189, 231, 123, 191, 15,  127, 111, 175, 79,  240, 143,
    230, 188, 242, 244, 119, 238, 190, 221, 247, 223, 254, 207,
    63,  103, 47,  174, 245, 250, 243, 249, 246, 252, 0,
};
const u8 wallmask[] = {
    0,  6,  0, 11, 13, 11, 15, 13, 3, 9,  0, 0, 9, 12, 6,  3,
    12, 15, 3, 7,  14, 15, 0,  15, 6, 12, 0, 0, 3, 6,  12, 9,
    0,  9,  0, 15, 15, 7,  15, 14, 0, 0,  0, 0, 0, 0,
};

// floor(35 / x) for x in [3,10]
const u8 other_dim[] = {11, 8, 7, 5, 5, 4, 3, 3};

// 54  0101 0100 UL
// d6  1101 0110 U
// 92  1001 0010 UR
// 7c  0111 1100 L
// ff  1111 1111 M
// b3  1011 0011 R
// 68  0110 1000 DL
// e9  1110 1001 D
// a1  1010 0001 DR
const u8 validmap[] = {
  0x54,0xd6,0xd6,0xd6,0xd6,0xd6,0xd6,0xd6,0xd6,0xd6,0xd6,0xd6,0xd6,0xd6,0xd6,0x92,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x68,0xe9,0xe9,0xe9,0xe9,0xe9,0xe9,0xe9,0xe9,0xe9,0xe9,0xe9,0xe9,0xe9,0xe9,0xa1,
};

// U           0b0010 0000   20
// L           0b1000 0000   80
// D           0b0001 0000   10
// R           0b0100 0000   40
// U UR:       0b0010 1000   28
// U UL:       0b0010 0001   21
// L UL:       0b1000 0001   81
// L DL:       0b1000 0010   82
// D DL:       0b0001 0010   12
// D DR:       0b0001 0100   14
// R DR:       0b0100 0100   44
// R UR:       0b0100 1000   48
// UL U  UR:   0b0010 1001   29
// L  UL U:    0b1010 0001   a1
// UL L  DL:   0b1000 0011   83
// L  DL D:    0b1001 0010   92
// DL D  DR:   0b0001 0110   16
// D  DR R:    0b0101 0100   54
// DR R  UR:   0b0100 1100   4c
// R  UR U:    0b0110 1000   68

const u8 sightsig[] = {                          // extended out to 16 wide
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  0, 0, 0, 0, 0, 0, 0,
  0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x00,  0, 0, 0, 0, 0, 0, 0,
  0x00,0x00,0x00,0x21,0x20,0x28,0x00,0x00,0x00,  0, 0, 0, 0, 0, 0, 0,
  0x00,0x00,0x81,0xa1,0x29,0x68,0x48,0x00,0x00,  0, 0, 0, 0, 0, 0, 0,
  0x00,0x80,0x80,0x83,0xff,0x4c,0x40,0x40,0x00,  0, 0, 0, 0, 0, 0, 0,
  0x00,0x00,0x82,0x92,0x16,0x54,0x44,0x00,0x00,  0, 0, 0, 0, 0, 0, 0,
  0x00,0x00,0x00,0x12,0x10,0x14,0x00,0x00,0x00,  0, 0, 0, 0, 0, 0, 0,
  0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,  0, 0, 0, 0, 0, 0, 0,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  0, 0, 0, 0, 0, 0, 0,
};

const u8 void_tiles[] = {TILE_VOID1, TILE_VOID1, TILE_VOID1, TILE_VOID1,
                         TILE_VOID1, TILE_VOID1, TILE_VOID2};
const u8 dirt_tiles[] = {TILE_EMPTY, TILE_DIRT1, TILE_DIRT2, TILE_DIRT3};
const u8 plant_tiles[] = {TILE_PLANT1, TILE_PLANT2, TILE_PLANT3};

const u8 room_permutations[][MAX_ROOMS] = {
    {0, 1, 2, 3}, {0, 1, 3, 2}, {0, 2, 1, 3}, {0, 2, 3, 1}, {0, 3, 1, 2},
    {0, 3, 2, 1}, {1, 0, 2, 3}, {1, 0, 3, 2}, {1, 2, 0, 3}, {1, 2, 3, 0},
    {1, 3, 0, 2}, {1, 3, 2, 0}, {2, 0, 1, 3}, {2, 0, 3, 1}, {2, 1, 0, 3},
    {2, 1, 3, 0}, {2, 3, 0, 1}, {2, 3, 1, 0}, {3, 0, 1, 2}, {3, 0, 2, 1},
    {3, 1, 0, 2}, {3, 1, 2, 0}, {3, 2, 0, 1}, {3, 2, 1, 0},
};

const u8 vase_mobs[] = {0, 0, 0, 0, 0, 0, MOB_TYPE_VASE1, MOB_TYPE_VASE2};
const u8 plant_mobs[] = {MOB_TYPE_WEED, MOB_TYPE_WEED, MOB_TYPE_BOMB};

const u8 mob_plan[] = {
    // floor 1
    MOB_TYPE_SLIME, MOB_TYPE_SLIME, MOB_TYPE_SLIME, MOB_TYPE_SLIME,
    MOB_TYPE_SLIME,
    // floor 2
    MOB_TYPE_SLIME, MOB_TYPE_SLIME, MOB_TYPE_QUEEN, MOB_TYPE_SLIME,
    MOB_TYPE_SLIME, MOB_TYPE_QUEEN,
    // floor 3
    MOB_TYPE_SLIME, MOB_TYPE_SLIME, MOB_TYPE_QUEEN, MOB_TYPE_SLIME,
    MOB_TYPE_SLIME, MOB_TYPE_QUEEN, MOB_TYPE_SLIME,
    // floor 4
    MOB_TYPE_SCORPION, MOB_TYPE_SCORPION, MOB_TYPE_SCORPION, MOB_TYPE_HULK,
    MOB_TYPE_SCORPION, MOB_TYPE_SCORPION, MOB_TYPE_SCORPION, MOB_TYPE_HULK,
    // floor 5
    MOB_TYPE_SCORPION, MOB_TYPE_SCORPION, MOB_TYPE_HULK, MOB_TYPE_SCORPION,
    MOB_TYPE_SCORPION, MOB_TYPE_HULK, MOB_TYPE_SCORPION, MOB_TYPE_SCORPION,
    MOB_TYPE_HULK,
    // floor 6
    MOB_TYPE_HULK, MOB_TYPE_SCORPION, MOB_TYPE_HULK, MOB_TYPE_SCORPION,
    MOB_TYPE_SCORPION, MOB_TYPE_HULK, MOB_TYPE_SCORPION, MOB_TYPE_SCORPION,
    MOB_TYPE_HULK,
    // floor 7
    MOB_TYPE_GHOST, MOB_TYPE_GHOST, MOB_TYPE_GHOST, MOB_TYPE_KONG,
    MOB_TYPE_GHOST, MOB_TYPE_GHOST, MOB_TYPE_GHOST, MOB_TYPE_KONG,
    MOB_TYPE_GHOST,
    // floor 8
    MOB_TYPE_GHOST, MOB_TYPE_GHOST, MOB_TYPE_KONG, MOB_TYPE_GHOST,
    MOB_TYPE_GHOST, MOB_TYPE_KONG, MOB_TYPE_GHOST, MOB_TYPE_GHOST,
    MOB_TYPE_KONG,
    // floor 9
    MOB_TYPE_GHOST, MOB_TYPE_KONG, MOB_TYPE_GHOST, MOB_TYPE_KONG,
    MOB_TYPE_GHOST, MOB_TYPE_KONG, MOB_TYPE_GHOST, MOB_TYPE_KONG,
    MOB_TYPE_GHOST};

const u8 mob_plan_floor[] = {
    0,  // floor 0
    0,  // floor 1
    5,  // floor 2
    11, // floor 3
    18, // floor 4
    26, // floor 5
    35, // floor 6
    44, // floor 7
    53, // floor 8
    62, // floor 9
    70, // total
};

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

const u8 mob_type_key[] = {
    0, // player
    1, // slime
    1, // queen
    1, // scorpion
    1, // hulk
    1, // ghost
    1, // kong
    0, // reaper
    0, // weed
    0, // bomb
    0, // vase1
    0, // vase2
    0, // chest
    0, // heart chest
};

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
    0xf1, // PICKUP_TYPE_JUMP,
    0xf2, // PICKUP_TYPE_BOLT,
    0xf3, // PICKUP_TYPE_PUSH,
    0xf4, // PICKUP_TYPE_GRAPPLE,
    0xf5, // PICKUP_TYPE_SPEAR,
    0xf6, // PICKUP_TYPE_SMASH,
    0xf7, // PICKUP_TYPE_HOOK,
    0xf8, // PICKUP_TYPE_SPIN,
    0xf9, // PICKUP_TYPE_SUPLEX,
    0xfa, // PICKUP_TYPE_SLAP,
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
    116, 228, 234, 117, 197, 229, 117, 208, 214, 217, 217, 220, 0,   229, 0,   118,
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

const u8 inventory_up_y[] = {
    40, 41, 41, 41, 41, 41, 41, 41, 42, 42, 42,  43,  43,  43, 44,
    44, 45, 45, 46, 47, 47, 48, 49, 50, 51, 53,  54,  56,  58, 60,
    62, 64, 67, 70, 74, 77, 82, 86, 91, 97, 104, 111, 119,
};

const u8 inventory_down_y[] = {
    128, 127, 127, 127, 127, 127, 127, 127, 126, 126, 126, 125, 125, 125, 124,
    124, 123, 123, 122, 121, 121, 120, 119, 118, 117, 115, 114, 112, 110, 108,
    106, 104, 101, 98,  94,  91,  86,  82,  77,  71,  64,  57,  49,
};

const u8 msg_tiles[] = {
  // "wurstlord awakens"
  0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63,
  0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64,
  // "one monster holds the key"
  0x65, 0x66, 0x67, 0x68, 0x5d, 0x69, 0x6a, 0x6b, 0x63,
  0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x6d, 0x6d, 0x72,
};

// Explosion sprites
const u8 boom_spr_speed[] = {40, 19, 20, 38, 41, 39, 38, 22,
                             39, 31, 15, 34, 22, 26, 36, 45};
const u8 boom_spr_anim_speed[] = {10, 5, 5, 10, 11, 10, 10, 6,
                                  10, 8, 4, 9,  6,  7,  9,  12};
const u16 boom_spr_x[] = {64665, 757, 65447, 65176, 841, 64448, 65167, 398,
                          49,    570, 1221,  65345, 807, 65415, 303,   351};
const u16 boom_spr_y[] = {125,   65459, 65415, 541,   64906, 65047, 270, 277,
                          65264, 65337, 65419, 65041, 65281, 785,   479, 65493};
const u16 boom_spr_dx[] = {65301, 177, 65459, 65409, 196, 65394, 65378, 202,
                           39,    166, 245,   65483, 243, 65498, 114,   135};
const u16 boom_spr_dy[] = {33,    65518, 65432, 190,   65389, 65472,
                           115,   140,   65318, 65477, 65512, 65400,
                           65459, 243,   180,   65519};

void init(void);
void do_turn(void);
void pass_turn(void);
void begin_animate(void);
void move_player(void);
void mobdir(u8 index, u8 dir);
void mobwalk(u8 index, u8 dir);
void mobbump(u8 index, u8 dir);
void mobhop(u8 index, u8 pos);
void pickhop(u8 index, u8 pos);
void hitmob(u8 index, u8 dmg);
void hitpos(u8 pos, u8 dmg);
void update_wall_face(u8 pos);
void do_ai(void);
void do_ai_weeds(void);
u8 do_mob_ai(u8 index);
u8 ai_dobump(u8 index);
u8 ai_tcheck(u8 index);
u8 ai_getnextstep(u8 index);
u8 ai_getnextstep_rev(u8 index);
void sight(void);
void blind(void);
void calcdist_ai(u8 from, u8 to);
void do_animate(void);
void end_animate(void);
void hide_float_sprites(void);
void set_tile_during_vbl(u8 pos, u8 tile);
void set_digit_tile_during_vbl(u16 addr, u8 value);
void set_tile_range_during_vbl(u16 addr, u8* source, u8 len);
void update_tile(u8 pos, u8 tile);
void update_tile_if_unfogged(u8 pos, u8 tile);
void unfog_tile(u8 pos);
void vbl_interrupt(void);

void trigger_step(u8 index);

void addfloat(u8 pos, u8 tile);
void update_floats_and_msg_and_sprs(void);
u16 drag(u16 x);
void showmsg(u8 index, u8 y);

void inv_animate(void);

void update_obj1_pal(void);
void fadeout(void);
void fadein(void);

void addmob(MobType type, u8 pos);
void delmob(u8 index);
void addpick(PickupType type, u8 pos);
void delpick(u8 index);
u8 is_valid(u8 pos, u8 diff) __preserves_regs(b, c);
void droppick(PickupType type, u8 pos);
void droppick_rnd(u8 pos);
u8 dropspot(u8 pos);

void adddeadmob(u8 index);

void initdtmap(void);

// Map generation
void mapgen(void);
void mapgeninit(void);
void copymap(u8 index);
void roomgen(void);
void mazeworms(void);
void update_carve1(u8 pos);
u8 nexttoroom4(u8 pos, u8 valid);
u8 nexttoroom8(u8 pos, u8 valid);
void digworm(u8 pos);
void carvedoors(void);
void update_door1(u8 pos);
void carvecuts(void);
void calcdist(u8 pos);
void startend(void);
u8 startscore(u8 pos);
void fillends(void);
void update_fill1(u8 pos);
void prettywalls(void);
void voids(void);
void chests(void);
void decoration(void);
void begin_tile_anim(void);
void end_tile_anim(void);
void add_tile_anim(u8 pos, u8 tile);
void nop_saw_anim(u8 pos);
void spawnmobs(void);
u8 no_mob_neighbors(u8 pos);
void mapset(u8* pos, u8 w, u8 h, u8 val);
void sigrect_empty(u8 pos, u8 w, u8 h);
void sigempty(u8 pos);
void sigwall(u8 pos);
u8 sigmatch(u16 pos, u8* sig, u8* mask);
u8 ds_union(u8 x, u8 y);
u8 ds_find(u8 id);
u8 getpos(u8 cand);
u8 randint(u8 mx);

Map tmap;        // Tile map (everything unfogged)
Map dtmap;       // Display tile map (w/ fogged tiles)
Map roommap;     // Room/void map
Map distmap;     // Distance map
Map mobmap;      // Mob map
Map pickmap;     // Pickup map
Map sigmap;      // Signature map (tracks neighbors) **only used during mapgen
Map tempmap;     // Temp map (used for carving)      **only used during mapgen
Map mobsightmap; // Always checks 4 steps from player
Map fogmap;      // Tiles player hasn't seen
Map sawmap;      // Map from saw to its animation code addr in tile_code

// The following data structures is used for rooms as well as union-find
// algorithm; see https://en.wikipedia.org/wiki/Disjoint-set_data_structure
Map ds_sets;                   // **only used during mapgen
u8 ds_parents[MAX_DS_SET];     // **only used during mapgen
u8 ds_sizes[MAX_DS_SET];       // **only used during mapgen
u8 ds_nextid;                  // **only used during mapgen
u8 ds_num_sets;                // **only used during mapgen
u8 dogate;                     // **only used during mapgen
u8 endpos, gatepos, heartpos;  // **only used during mapgen

u8 room_pos[MAX_ROOMS];
u8 room_w[MAX_ROOMS];
u8 room_h[MAX_ROOMS];
u8 room_avoid[MAX_ROOMS]; // ** only used during mapgen
u8 num_rooms;

u8 void_num_tiles[MAX_VOIDS]; // Number of tiles in a void region
u8 void_exit[MAX_VOIDS];      // Exit tile for a given void region
u8 num_voids;

u8 start_room;
u8 floor, floor_tile[2];
u8 startpos;

u8 cands[MAX_CANDS];
u8 num_cands;

MobType mob_type[MAX_MOBS];
u8 mob_anim_frame[MAX_MOBS]; // Index into mob_type_anim_frames
u8 mob_anim_timer[MAX_MOBS]; // 0..mob_anim_speed[type], how long between frames
u8 mob_anim_speed[MAX_MOBS]; // current anim speed (changes when moving)
u8 mob_pos[MAX_MOBS];
u8 mob_x[MAX_MOBS], mob_y[MAX_MOBS];   // x,y location of sprite
u8 mob_dx[MAX_MOBS], mob_dy[MAX_MOBS]; // dx,dy moving sprite
u8 mob_move_timer[MAX_MOBS];           // timer for sprite animation
MobAnimState mob_anim_state[MAX_MOBS]; // sprite animation state
u8 mob_flip[MAX_MOBS];                 // 0=facing right 1=facing left
MobAI mob_task[MAX_MOBS];              // AI routine
u8 mob_target_pos[MAX_MOBS];           // where the mob last saw the player
u8 mob_ai_cool[MAX_MOBS]; // cooldown time while mob is searching for player
u8 mob_active[MAX_MOBS];  // 0=inactive 1=active
u8 mob_charge[MAX_MOBS];  // only used by queen
u8 mob_hp[MAX_MOBS];
u8 mob_flash[MAX_MOBS];
u8 num_mobs;
u8 key_mob;

u8 dmob_x[MAX_DEAD_MOBS];
u8 dmob_y[MAX_DEAD_MOBS];
u8 dmob_tile[MAX_DEAD_MOBS];
u8 dmob_prop[MAX_DEAD_MOBS];
u8 dmob_timer[MAX_DEAD_MOBS];
u8 num_dead_mobs;

PickupType pick_type[MAX_PICKUPS];
u8 pick_pos[MAX_PICKUPS];
u8 pick_anim_frame[MAX_PICKUPS];             // Index into pick_type_anim_frames
u8 pick_anim_timer[MAX_PICKUPS];             // 0..pick_type_anim_speed[type]
u8 pick_x[MAX_PICKUPS], pick_y[MAX_PICKUPS]; // x,y location of sprite
u8 pick_dx[MAX_PICKUPS], pick_dy[MAX_PICKUPS]; // dx,dy moving sprite
u8 pick_move_timer[MAX_PICKUPS];               // timer for sprite animation
u8 num_picks;

SprType spr_type[MAX_SPRS];
u8 spr_anim_frame[MAX_SPRS];            // Actual sprite tile (not index)
u8 spr_anim_timer[MAX_SPRS];            // 0..spr_anim_speed
u8 spr_anim_speed[MAX_SPRS];            //
u16 spr_x[MAX_SPRS], spr_y[MAX_SPRS];   // 8.8 fixed point
u16 spr_dx[MAX_SPRS], spr_dy[MAX_SPRS]; // 8.8 fixed point
u8 spr_timer[MAX_SPRS];
u8 spr_drag[MAX_SPRS];
u8 num_sprs;

u8 tile_code[TILE_CODE_SIZE];         // code for animating tiles
u8 mob_tile_code[MOB_TILE_CODE_SIZE]; // code for animating mobs
u8 *tile_code_ptr;                    // next available index in tile_code
u16 last_tile_addr;                   // last tile address in HL for tile_code
u8 last_tile_val;                     // last tile value in A for tile_code
u8 tile_timer; // timer for animating tiles (every TILE_ANIM_FRAMES frames)
u8 tile_inc;   // either 0 or TILE_ANIM_FRAME_DIFF, for two frames of animation
u8 *mob_tile_code_ptr;  // next available index in mob_tile_code
u16 mob_last_tile_addr; // last tile address in HL for mob_tile_code
u8 mob_last_tile_val;   // last tile value in A for mob_tile_code

u8 float_time[MAX_FLOATS];
u8 next_float;

u8 msg_timer;

Turn turn;
u8 noturn;

u8 joy, lastjoy, newjoy;

u8 doupdatemap, donextlevel, doblind, dosight;
OAM_item_t *next_sprite, *last_next_sprite;

u8 inv_anim_up;
u8 inv_anim_timer;
u8 inv_select;
u8 inv_select_timer;
u8 inv_select_frame;
u8 inv_msg_update;

u8 equip_type[MAX_EQUIPS];
u8 equip_charge[MAX_EQUIPS];
u8 num_keys;
u8 recover; // how long until recovering from blind
u16 steps;

u8 obj_pal1_timer, obj_pal1_index;

u16 myclock;
void tim_interrupt(void) { ++myclock; }

void main(void) {
  init();
  mapgen();

  doupdatemap = 1;
  LCDC_REG = 0b11100011;  // display on, window/bg/obj on, window@9c00
  fadein();

  while(1) {
    lastjoy = joy;
    joy = joypad();
    newjoy = (joy ^ lastjoy) & joy;

    if (donextlevel || (joy & J_START)) {
      donextlevel = 0;
      hide_float_sprites();

      if (++floor == 11) { floor = 0; }
      fadeout();
      IE_REG &= ~VBL_IFLAG;
      mapgen();
      // Reset the animations so they don't draw on this new map
      begin_animate();
      end_animate();
      if (floor == 10) {
        floor_tile[0] = TILE_1;
        floor_tile[1] = TILE_0;
      } else {
        floor_tile[0] = TILE_0 + floor;
        floor_tile[1] = 0;
      }
      IE_REG |= VBL_IFLAG;
      doupdatemap = 1;
      fadein();
    }

    begin_animate();
    do_turn();
    do_animate();
    update_floats_and_msg_and_sprs();
    inv_animate();
    end_animate();
    update_obj1_pal();
    wait_vbl_done();
  }
}

extern u16 s__DATA, l__DATA;

void init(void) {
  DISPLAY_OFF;
  // TODO: clear WRAM to 0

  // 0:LightGray 1:DarkGray 2:Black 3:White
  BGP_REG = OBP0_REG = OBP1_REG = fadepal[0];
  obj_pal1_timer = OBJ1_PAL_FRAMES;
  obj_pal1_index = 0;

  add_TIM(tim_interrupt);
  TMA_REG = 0;      // Divide clock by 256 => 16Hz
  TAC_REG = 0b100;  // 4096Hz, timer on.
  IE_REG |= TIM_IFLAG;

  enable_interrupts();

  floor = 1;
  initrand(0x1234);  // TODO: seed with DIV on button press

  gb_decompress_bkg_data(0, bg_bin);
  gb_decompress_bkg_data(0x80, shared_bin);
  gb_decompress_sprite_data(0, sprites_bin);
  set_win_tiles(0, 0, INV_WIDTH, INV_HEIGHT, inventory_map);
  init_bkg(0);

  WX_REG = 23;
  WY_REG = 128;

  num_picks = 0;
  num_mobs = 0;
  num_dead_mobs = 0;
  addmob(MOB_TYPE_PLAYER, 0);

  doupdatemap = 0;
  donextlevel = 0;
  doblind = 0;
  dosight = 0;

  floor_tile[0] = TILE_0;
  floor_tile[1] = 0;

  turn = TURN_PLAYER;
  noturn = 0;

  next_float = BASE_FLOAT;

  inv_anim_up = 0;
  inv_anim_timer = 0;
  inv_select = 0;
  inv_select_timer = INV_SELECT_FRAMES;
  inv_select_frame = 0;
  inv_msg_update = 1;

  equip_type[0] = equip_type[1] = equip_type[2] = equip_type[3] = 0;
  equip_charge[0] = equip_charge[1] = equip_charge[2] = equip_charge[3] = 0;

  tile_inc = 0;
  tile_timer = TILE_ANIM_FRAMES;
  tile_code[0] = mob_tile_code[0] = 0xc9; // ret
  add_VBL(vbl_interrupt);
}

void begin_animate(void) {
  mob_last_tile_addr = 0;
  mob_last_tile_val = 0xff;
  mob_tile_code_ptr = mob_tile_code;
  // Initialize to ret so if a VBL occurs while we're setting the animation it
  // won't run off the end
  *mob_tile_code_ptr++ = 0xc9; // ret

  // Start mob sprites after floats
  next_sprite = shadow_OAM + BASE_FLOAT + MAX_FLOATS;
}

void end_animate(void) {
  // Hide the rest of the sprites
  while (next_sprite < last_next_sprite) {
    next_sprite->y = 0;  // hide sprite
    ++next_sprite;
  }
  last_next_sprite = next_sprite;

  // When blinded, don't update any mob animations
  if (doblind) {
    doblind = 0;
  } else {
    *mob_tile_code_ptr++ = 0xf1; // pop af
    *mob_tile_code_ptr++ = 0xc9; // ret
    mob_tile_code[0] = 0xf5;     // push af
  }
}

void hide_float_sprites(void) {
  u8 i;
  for (i = BASE_FLOAT; i < next_float; ++i) {
    hide_sprite(i);
  }
  next_float = BASE_FLOAT;
}

void do_turn(void) {
  if (inv_anim_up) {
    if (newjoy & J_UP) {
      inv_select = (inv_select + 3) & 3;
      inv_msg_update = 1;
    } else if (newjoy & J_DOWN) {
      inv_select = (inv_select + 1) & 3;
      inv_msg_update = 1;
    }

    if ((newjoy & J_B) && !inv_anim_timer) {
      inv_anim_timer = INV_ANIM_FRAMES;
      inv_anim_up ^= 1;
    }
  } else {
    switch (turn) {
    case TURN_PLAYER:
      move_player();
      break;

    case TURN_AI:
      turn = TURN_AI_WAIT;
      do_ai();
      break;

    case TURN_WEEDS:
      turn = TURN_WEEDS_WAIT;
      do_ai();
      break;
    }
  }
}

void pass_turn(void) {
  switch (turn) {
  case TURN_PLAYER_WAIT:
    if (noturn) {
      turn = TURN_PLAYER;
      noturn = 0;
    } else {
      turn = TURN_AI;
    }
    break;

  case TURN_AI_WAIT:
    turn = TURN_WEEDS;
    break;

  case TURN_WEEDS_WAIT:
    turn = TURN_PLAYER;
    if (floor && steps == REAPER_AWAKENS_STEPS) {
      ++steps; // Make sure to only spawn reaper once per floor
      addmob(MOB_TYPE_REAPER, dropspot(startpos));
      mob_active[num_mobs - 1] = 1;
      showmsg(MSG_REAPER, MSG_REAPER_Y);
    }
    if (recover && --recover == 0) {
      dosight = 1;
    }
    break;
  }

  if (dosight) {
    dosight = 0;
    sight();
  }
}

void move_player(void) {
  u8 dir, pos, newpos, tile;
  if (mob_move_timer[PLAYER_MOB] == 0) {
    if (joy & (J_LEFT | J_RIGHT | J_UP | J_DOWN)) {
      if (joy & J_LEFT) {
        dir = DIR_LEFT;
      } else if (joy & J_RIGHT) {
        dir = DIR_RIGHT;
      } else if (joy & J_UP) {
        dir = DIR_UP;
      } else {
        dir = DIR_DOWN;
      }

      pos = mob_pos[PLAYER_MOB];
      newpos = POS_DIR(pos, dir);
      mobdir(PLAYER_MOB, dir);
      if (IS_MOB(newpos)) {
        mobbump(PLAYER_MOB, dir);
        if (mob_type[mobmap[newpos] - 1] == MOB_TYPE_SCORPION &&
            URAND_50_PERCENT()) {
          blind();
        }
        hitmob(mobmap[newpos] - 1, 1);
        goto done;
      } else if (validmap[pos] & dirvalid[dir]) {
        tile = tmap[newpos];
        if (IS_WALL_TILE(tile)) {
          if (IS_SPECIAL_TILE(tile)) {
            if (tile == TILE_GATE) {
              if (num_keys) {
                --num_keys;

                update_tile(newpos, TILE_EMPTY);
                set_digit_tile_during_vbl(INV_KEYS_ADDR, num_keys);
                dosight = 1;
                goto dobump;
              } else {
                showmsg(MSG_KEY, MSG_KEY_Y);
              }
            } else if (tile == TILE_VOID_BUTTON_U ||
                       tile == TILE_VOID_BUTTON_L ||
                       tile == TILE_VOID_BUTTON_R ||
                       tile == TILE_VOID_BUTTON_D) {
              update_tile(newpos, tile + TILE_VOID_OPEN_DIFF);
              update_wall_face(newpos);
              dosight = 1;
              goto dobump;
            }
          }
#if 0
          // XXX: bump to destroy walls for testing
          else {
            update_tile(newpos, TILE_EMPTY);
            goto dobump;
          }
#endif
          // fallthrough to bump below...
        } else {
          mobwalk(PLAYER_MOB, dir);
          ++steps;
          goto done;
        }
      }

      // Bump w/o taking a turn
      noturn = 1;
    dobump:
      mobbump(PLAYER_MOB, dir);
    done:
      turn = TURN_PLAYER_WAIT;
    }

    // Open inventory
    if ((newjoy & J_B) && !inv_anim_timer) {
      inv_anim_timer = INV_ANIM_FRAMES;
      inv_anim_up ^= 1;
    }
  }
}

void mobdir(u8 index, u8 dir) {
  if (dir == DIR_LEFT) {
    mob_flip[index] = 1;
  } else if (dir == DIR_RIGHT) {
    mob_flip[index] = 0;
  }
}

void mobwalk(u8 index, u8 dir) {
  u8 pos, newpos;

  pos = mob_pos[index];
  mob_x[index] = POS_TO_X(pos);
  mob_y[index] = POS_TO_Y(pos);
  mob_dx[index] = dirx[dir];
  mob_dy[index] = diry[dir];
  set_tile_during_vbl(pos, dtmap[pos]);

  mob_move_timer[index] = WALK_FRAMES;
  mob_anim_state[index] = MOB_ANIM_STATE_WALK;
  mob_pos[index] = newpos = POS_DIR(pos, dir);
  mob_anim_speed[index] = mob_anim_timer[index] = 3;
  mobmap[pos] = 0;
  mobmap[newpos] = index + 1;
}

void mobbump(u8 index, u8 dir) {
  u8 pos = mob_pos[index];
  mob_x[index] = POS_TO_X(pos);
  mob_y[index] = POS_TO_Y(pos);
  mob_dx[index] = dirx[dir];
  mob_dy[index] = diry[dir];
  set_tile_during_vbl(pos, dtmap[pos]);

  mob_move_timer[index] = BUMP_FRAMES;
  mob_anim_state[index] = MOB_ANIM_STATE_BUMP1;
}

void mobhop(u8 index, u8 newpos) {
  u8 pos, oldx, oldy, newx, newy;
  pos = mob_pos[index];
  mob_x[index] = oldx = POS_TO_X(pos);
  mob_y[index] = oldy = POS_TO_Y(pos);
  newx = POS_TO_X(newpos);
  newy = POS_TO_Y(newpos);

  mob_dx[index] = (s8)(newx - oldx) >> 3;
  mob_dy[index] = (s8)(newy - oldy) >> 3;

  mob_move_timer[index] = MOB_HOP_FRAMES;
  mob_anim_state[index] = MOB_ANIM_STATE_HOP4;
  mob_pos[index] = newpos;
  mobmap[pos] = 0;
  mobmap[newpos] = index + 1;
}

void pickhop(u8 index, u8 newpos) {
  u8 pos, oldx, oldy, newx, newy;
  pos = pick_pos[index];
  pick_x[index] = oldx = POS_TO_X(pos);
  pick_y[index] = oldy = POS_TO_Y(pos);
  newx = POS_TO_X(newpos);
  newy = POS_TO_Y(newpos);

  pick_dx[index] = (s8)(newx - oldx) >> 3;
  pick_dy[index] = (s8)(newy - oldy) >> 3;

  pick_move_timer[index] = PICK_HOP_FRAMES;
  pick_pos[index] = newpos;
  pickmap[pos] = 0;
  pickmap[newpos] = index + 1;
}

void hitmob(u8 index, u8 dmg) {
  u8 mob, mtype, percent, tile, slime, pos, valid, i;
  u16 x, y;
  pos = mob_pos[index];
  mtype = mob_type[index];

  if (mob_type_object[mtype]) {
    if (mtype == MOB_TYPE_CHEST) {
      percent = 100;
      tile = TILE_CHEST_OPEN;
      slime = 0;
    } else if (mtype == MOB_TYPE_BOMB) {
      percent = 100;
      tile = TILE_BOMB_EXPLODED;
      slime = 0;
    } else {
      percent = 20;
      tile = dirt_tiles[URAND() & 3];
      slime = 1;
    }

    delmob(index);
    if (!IS_SPECIAL_TILE(tmap[pos])) {
      update_tile(pos, tile);
    }

    if (randint(100) < percent) {
      if (slime && URAND_20_PERCENT()) {
        mob = num_mobs;
        addmob(MOB_TYPE_SLIME, pos);
        mobhop(mob, dropspot(pos));
      } else if (mtype == MOB_TYPE_HEART_CHEST && URAND_20_PERCENT()) {
        droppick(PICKUP_TYPE_HEART, pos);
      } else {
        droppick_rnd(pos);
      }
    }

    // Explode bomb after the mob is deleted (to prevent infinite recursion)
    if (mtype == MOB_TYPE_BOMB) {
      // Destroy the surrounding tiles
      valid = validmap[pos];
      u8 dir = 7;
      do {
        if (valid & dirvalid[dir]) {
          u8 newpos = POS_DIR(pos, dir);
          u8 tile = tmap[newpos];
          if (IS_WALL_FACE_TILE(tile)) {
            tile = TILE_WALL_FACE_RUBBLE;
          } else if (IS_UNSPECIAL_WALL_TILE(tile)) {
            tile = dirt_tiles[URAND() & 3];
          } else {
            goto noupdate;
          }
          update_tile(newpos, tile);
        noupdate:
          hitpos(newpos, dmg);
        }
      } while (dir--);
      update_tile(pos, TILE_BOMB_EXPLODED);

      x = POS_TO_X(pos) << 8;
      y = POS_TO_Y(pos) << 8;

      for (i = 0; i < sizeof(boom_spr_speed); ++i) {
        spr_type[num_sprs] = SPR_TYPE_BOOM;
        spr_anim_frame[num_sprs] = TILE_BOOM;
        spr_anim_timer[num_sprs] = spr_anim_speed[num_sprs] =
            boom_spr_anim_speed[i];
        spr_x[num_sprs] = x + boom_spr_x[i];
        spr_y[num_sprs] = y + boom_spr_y[i];
        spr_dx[num_sprs] = boom_spr_dx[i];
        spr_dy[num_sprs] = boom_spr_dy[i];
        spr_drag[num_sprs] = 1;
        spr_timer[num_sprs] = boom_spr_speed[i];
        ++num_sprs;
      }
    }
  } else {
    addfloat(pos, float_dmg[dmg]);

    if (mob_hp[index] <= dmg) {
      adddeadmob(index);
      // drop key, if any.
      // NOTE: This code is subtle; delmob() will update the
      // key_mob, and droppick() must be called after delmob() so the pickup
      // can be dropped where the mob was.
      if (index + 1 == key_mob) {
        delmob(index);
        droppick(PICKUP_TYPE_KEY, pos);
      } else {
        delmob(index);
      }
      // TODO: restore to 5hp if reaper killed, and drop key
      set_tile_during_vbl(pos, dtmap[pos]);
    } else {
      mob_flash[index] = MOB_FLASH_FRAMES;
      mob_hp[index] -= dmg;
      if (index == PLAYER_MOB) {
        set_digit_tile_during_vbl(INV_HP_ADDR, mob_hp[PLAYER_MOB]);
      }
    }
  }
}

void hitpos(u8 pos, u8 dmg) {
  u8 tile, mob;
  tile = tmap[pos];
  if ((mob = mobmap[pos])) {
    hitmob(mob - 1, dmg); // XXX maybe recursive...
  }
  if (tmap[pos] == TILE_SAW) {
    nop_saw_anim(pos);
  } else if (IS_CRACKED_WALL_TILE(tile)) {
    tile = dirt_tiles[URAND() & 3];
    update_wall_face(pos);
  } else {
    goto noupdate;
  }
  update_tile(pos, tile);
noupdate:
  dosight = 1;
}

void update_wall_face(u8 pos) {
  if (validmap[pos] & VALID_D) {
    pos = POS_D(pos);
    u8 tile = tmap[pos];
    if (tile == TILE_WALL_FACE || tile == TILE_WALL_FACE_CRACKED) {
      tile = TILE_EMPTY;
    } else if (tile == TILE_WALL_FACE_RUBBLE) {
      tile = dirt_tiles[URAND() & 3];
    } else if (tile == TILE_WALL_FACE_PLANT) {
      tile = TILE_PLANT1;
    }
    update_tile_if_unfogged(pos, tile);
  }
}

void do_ai(void) {
  u8 moving = 0;
  u8 i;
  for (i = 0; i < num_mobs; ++i) {
    if ((mob_type[i] == MOB_TYPE_WEED) == (turn == TURN_WEEDS_WAIT)) {
      moving |= do_mob_ai(i);
    }
  }
  if (!moving) {
    pass_turn();
  }
}

u8 do_mob_ai(u8 index) {
  u8 pos, mob, dir, valid;
  pos = mob_pos[index];
  switch (mob_task[index]) {
    case MOB_AI_NONE:
      break;

    case MOB_AI_WAIT:
      if (mobsightmap[pos]) {
        addfloat(pos, FLOAT_FOUND);
        mob_task[index] = mob_type_ai_active[mob_type[index]];
        mob_target_pos[index] = mob_pos[PLAYER_MOB];
        mob_ai_cool[index] = 0;
        mob_active[index] = 1;
      }
      break;

    case MOB_AI_WEED:
      valid = validmap[pos];
      if ((valid & VALID_L) && (mob = mobmap[POS_L(pos)])) {
        dir = 0;
      } else if ((valid & VALID_R) && (mob = mobmap[POS_R(pos)])) {
        dir = 1;
      } else if ((valid & VALID_U) && (mob = mobmap[POS_U(pos)])) {
        dir = 2;
      } else if ((valid & VALID_D) && (mob = mobmap[POS_D(pos)])) {
        dir = 3;
      } else {
        return 0;
      }
      mobbump(index, dir);
      hitmob(mob - 1, 1);
      return 1;

    case MOB_AI_REAPER:
      if (ai_dobump(index)) {
        return 1;
      } else {
        mob_target_pos[index] = mob_pos[PLAYER_MOB];
        dir = ai_getnextstep(index);
        if (dir != 255) {
          mobwalk(index, dir);
          return 1;
        }
      }
      break;

    case MOB_AI_ATTACK:
      if (ai_dobump(index)) {
        return 1;
      } else if (ai_tcheck(index)) {
        dir = ai_getnextstep(index);
        if (dir != 255) {
          mobwalk(index, dir);
          return 1;
        }
      }
      break;

    case MOB_AI_QUEEN:
      if (!mobsightmap[pos]) {
        mob_task[index] = MOB_AI_WAIT;
        mob_active[index] = 0;
        addfloat(pos, FLOAT_LOST);
      } else {
        mob_target_pos[index] = mob_pos[PLAYER_MOB];
        if (mob_charge[index] == 0) {
          dir = ai_getnextstep(index);
          if (dir != 255) {
            mob = num_mobs;
            addmob(MOB_TYPE_SLIME, pos);
            mobhop(mob, POS_DIR(pos, dir));
            mobmap[pos] = index + 1; // Fix mobmap back to queen
            mob_charge[index] = QUEEN_CHARGE_MOVES;
            return 1;
          }
        } else {
          --mob_charge[index];
          dir = ai_getnextstep_rev(index);
          if (dir != 255) {
            mobwalk(index, dir);
            return 1;
          }
        }
      }
      break;

    case MOB_AI_KONG:
      break;
  }
  return 0;
}

u8 ai_dobump(u8 index) {
  u8 pos, diff, dir;
  pos = mob_pos[index];
  diff = mob_pos[PLAYER_MOB] - pos;

  // Use mobsightmap as a quick check to determine whether this mob is close to
  // the player.
  if (mobsightmap[pos]) {
    if (diff == 0xff) { // left
      dir = 0;
    } else if (diff == 1) { // right
      dir = 1;
    } else if (diff == 0xf0) { // up
      dir = 2;
    } else if (diff == 0x10) { // down
      dir = 3;
    } else {
      return 0;
    }

    mobbump(index, dir);
    hitmob(PLAYER_MOB, 1);
    if (mob_type[index] == MOB_TYPE_SCORPION) {
      blind();
    }
    return 1;
  }
  return 0;
}

u8 ai_tcheck(u8 index) {
  ++mob_ai_cool[index];
  if (mobsightmap[mob_pos[index]]) {
    mob_target_pos[index] = mob_pos[PLAYER_MOB];
    mob_ai_cool[index] = 0;
  }
  if (mob_target_pos[index] == mob_pos[index] ||
      mob_ai_cool[index] > AI_COOL_MOVES) {
    addfloat(mob_pos[index], FLOAT_LOST);
    mob_task[index] = MOB_AI_WAIT;
    mob_active[index] = 0;
    return 0;
  }
  return 1;
}

u8 ai_getnextstep(u8 index) {
  u8 pos, newpos, bestval, bestdir, dist, valid;
  pos = mob_pos[index];
  calcdist_ai(pos, mob_target_pos[index]);
  bestval = bestdir = 255;
  valid = validmap[pos];

  newpos = POS_L(pos);
  if ((valid & VALID_L) && !IS_SMARTMOB(tmap[newpos], newpos) &&
      (dist = distmap[newpos]) && dist < bestval) {
    bestval = dist;
    bestdir = 0;
  }
  newpos = POS_R(pos);
  if ((valid & VALID_R) && !IS_SMARTMOB(tmap[newpos], newpos) &&
      (dist = distmap[newpos]) && dist < bestval) {
    bestval = dist;
    bestdir = 1;
  }
  newpos = POS_U(pos);
  if ((valid & VALID_U) && !IS_SMARTMOB(tmap[newpos], newpos) &&
      (dist = distmap[newpos]) && dist < bestval) {
    bestval = dist;
    bestdir = 2;
  }
  newpos = POS_D(pos);
  if ((valid & VALID_D) && !IS_SMARTMOB(tmap[newpos], newpos) &&
      (dist = distmap[newpos]) && dist < bestval) {
    bestval = dist;
    bestdir = 3;
  }
  return bestdir;
}

// TODO: combine with above?
u8 ai_getnextstep_rev(u8 index) {
  u8 pos, newpos, bestval, bestdir, dist, valid;
  pos = mob_pos[index];
  calcdist_ai(pos, mob_target_pos[index]);
  bestval = 0;
  bestdir = 255;
  valid = validmap[pos];

  newpos = POS_L(pos);
  if ((valid & VALID_L) && !IS_SMARTMOB(tmap[newpos], newpos) &&
      (dist = distmap[newpos]) && dist > bestval) {
    bestval = dist;
    bestdir = 0;
  }
  newpos = POS_R(pos);
  if ((valid & VALID_R) && !IS_SMARTMOB(tmap[newpos], newpos) &&
      (dist = distmap[newpos]) && dist > bestval) {
    bestval = dist;
    bestdir = 1;
  }
  newpos = POS_U(pos);
  if ((valid & VALID_U) && !IS_SMARTMOB(tmap[newpos], newpos) &&
      (dist = distmap[newpos]) && dist > bestval) {
    bestval = dist;
    bestdir = 2;
  }
  newpos = POS_D(pos);
  if ((valid & VALID_D) && !IS_SMARTMOB(tmap[newpos], newpos) &&
      (dist = distmap[newpos]) && dist > bestval) {
    bestval = dist;
    bestdir = 3;
  }
  return bestdir;
}

void sight(void) {
  u8 ppos, pos, adjpos, diff, head, oldtail, newtail, sig, valid, first,
      dist, maxdist;
  memset(mobsightmap, 0, sizeof(mobsightmap));

  // Put a "ret" at the beginning of the tile animation code in case it is
  // executed while we are modifying it.
  tile_code[0] = 0xc9;

  ppos = mob_pos[PLAYER_MOB];

  cands[head = 0] = 0;
  newtail = 1;
  first = 1;
  dist = 0;

  if (recover) {
    valid = validmap[ppos];
    if (valid & VALID_U) { cands[newtail++] = POS_U(0); }
    if (valid & VALID_L) { cands[newtail++] = POS_L(0); }
    if (valid & VALID_R) { cands[newtail++] = POS_R(0); }
    if (valid & VALID_D) { cands[newtail++] = POS_D(0); }
    maxdist = 1;
  } else {
    maxdist = 255;
  }

  do {
    oldtail = newtail;
    do {
      diff = cands[head++];
      mobsightmap[pos = (u8)(ppos + diff)] = 1;

      // Unfog this tile, within player sight range
      if (dist < maxdist && fogmap[pos]) {
        unfog_tile(pos);
        fogmap[pos] = 0;

        // Potentially start animating this tile
        if (IS_ANIMATED_TILE(tmap[pos])) {
          add_tile_anim(pos, tmap[pos]);

          // Store the offset into tile_code for this animated saw
          if (tmap[pos] == TILE_SAW) {
            sawmap[pos] = tile_code_ptr - tile_code - 1;
          }
        }
      }

      if (first || !IS_OPAQUE_TILE(tmap[pos])) {
        first = 0;
        valid = validmap[pos];

        // Unfog neighboring walls, within player sight range
        if (dist < maxdist) {
          if ((valid & VALID_U) && fogmap[adjpos = POS_U(pos)] &&
              IS_WALL_TILE(tmap[adjpos])) {
            unfog_tile(adjpos);
            fogmap[adjpos] = 0;
          }
          if ((valid & VALID_L) && fogmap[adjpos = POS_L(pos)] &&
              IS_WALL_TILE(tmap[adjpos])) {
            unfog_tile(adjpos);
            fogmap[adjpos] = 0;
          }
          if ((valid & VALID_R) && fogmap[adjpos = POS_R(pos)] &&
              IS_WALL_TILE(tmap[adjpos])) {
            unfog_tile(adjpos);
            fogmap[adjpos] = 0;
          }
          if ((valid & VALID_D) && fogmap[adjpos = POS_D(pos)] &&
              IS_WALL_TILE(tmap[adjpos])) {
            unfog_tile(adjpos);
            fogmap[adjpos] = 0;
          }
        }

        sig = sightsig[(u8)(68 + diff)] & valid;
        if (sig & VALID_UL) { cands[newtail++] = POS_UL(diff); }
        if (sig & VALID_U)  { cands[newtail++] = POS_U (diff); }
        if (sig & VALID_UR) { cands[newtail++] = POS_UR(diff); }
        if (sig & VALID_L)  { cands[newtail++] = POS_L (diff); }
        if (sig & VALID_R)  { cands[newtail++] = POS_R (diff); }
        if (sig & VALID_DL) { cands[newtail++] = POS_DL(diff); }
        if (sig & VALID_D)  { cands[newtail++] = POS_D (diff); }
        if (sig & VALID_DR) { cands[newtail++] = POS_DR(diff); }
      }
    } while(head != oldtail);
    ++dist;
  } while (oldtail != newtail);

  // Put the epilog back on, and replace the "ret" with "push af"
  end_tile_anim();
  tile_code[0] = 0xf5;
}

void blind(void) {
  u8 i, x, y;
  if (!recover) {
    recover = RECOVER_MOVES;

    // Display float
    x = POS_TO_X(mob_pos[PLAYER_MOB]) + FLOAT_BLIND_X_OFFSET;
    y = POS_TO_Y(mob_pos[PLAYER_MOB]);
    for (i = 0; i < 3; ++i) {
      shadow_OAM[next_float].x = x;
      shadow_OAM[next_float].y = y;
      shadow_OAM[next_float].tile = TILE_BLIND + i;
      float_time[next_float - BASE_FLOAT] = FLOAT_FRAMES;
      ++next_float;
      x += 8;
    }

    // Reset fogmap, dtmap and sawmap
    memset(fogmap, 1, sizeof(fogmap));
    memset(dtmap, 0, sizeof(fogmap));
    memset(sawmap, 0, sizeof(sawmap));

    // Reset all tile animations (e.g. saws, torches)
    begin_tile_anim();
    end_tile_anim();

    // Use sight() instead of dosight=1, since the tile updates will be removed
    sight();
    doupdatemap = 1;
    // Clear all the mob updates later (since we may be updating all the mobs
    // right now)
    doblind = 1;
  }
}

void calcdist_ai(u8 from, u8 to) {
  u8 pos, head, oldtail, newtail, valid, dist, newpos, maxdist;
  cands[head = 0] = pos = to;
  newtail = 1;

  memset(distmap, 0, sizeof(distmap));
  dist = 0;
  maxdist = 255;

  do {
    oldtail = newtail;
    ++dist;
    do {
      pos = cands[head++];
      if (pos == from) { maxdist = dist + 1; }
      if (!distmap[pos]) {
        distmap[pos] = dist;
        valid = validmap[pos];
        if ((valid & VALID_U) && !distmap[newpos = POS_U(pos)]) {
          if (!IS_MOB_AI(tmap[newpos], newpos)) { cands[newtail++] = newpos; }
        }
        if ((valid & VALID_L) && !distmap[newpos = POS_L(pos)]) {
          if (!IS_MOB_AI(tmap[newpos], newpos)) { cands[newtail++] = newpos; }
        }
        if ((valid & VALID_R) && !distmap[newpos = POS_R(pos)]) {
          if (!IS_MOB_AI(tmap[newpos], newpos)) { cands[newtail++] = newpos; }
        }
        if ((valid & VALID_D) && !distmap[newpos = POS_D(pos)]) {
          if (!IS_MOB_AI(tmap[newpos], newpos)) { cands[newtail++] = newpos; }
        }
      }
    } while (head != oldtail);
  } while (oldtail != newtail && dist != maxdist);
}

void do_animate(void) {
  u8 i, dosprite, dotile, sprite, prop, frame, animdone;
  animdone = num_sprs == 0;

  // use u8* pointer instead of OAM_item_t* pointer for better code generation
  u8* psprite = (u8*)next_sprite;

  // Loop through all pickups
  for (i = 0; i < num_picks; ++i) {
    // Don't draw the pickup if it is fogged or a mob is standing on top of it
    if (fogmap[pick_pos[i]] || mobmap[pick_pos[i]]) { continue; }

    dotile = 0;
    if (tile_timer == 1 && IS_ANIMATED_TILE(tmap[pick_pos[i]])) {
      dotile = 1;
    }

    sprite = pick_type_sprite_tile[pick_type[i]];
    if (--pick_anim_timer[i] == 0) {
      pick_anim_timer[i] = PICKUP_FRAMES;

      // bounce the sprite (if not a heart or key)
      if (sprite) {
        pick_y[i] += pickbounce[pick_anim_frame[i] & 7];
      }

      // All pickups have 8 frames
      if ((++pick_anim_frame[i] & 7) == 0) {
        pick_anim_frame[i] -= 8;
      }

      dotile = 1;
    }

    // Don't display pickups if the window would cover it; the bottom of the
    // sprite would be pick_y[i] + 8, but sprites are offset by 16 already (as
    // required by the hardware), so the actual position is pick_y[i] - 8.
    // Since the pickups bounce by 1 pixel up and down, we also display them at
    // WY_REG + 7 so they don't disappear while bouncing.
    if (sprite && pick_y[i] <= WY_REG + 9) {
      *psprite++ = pick_y[i];
      *psprite++ = pick_x[i];
      *psprite++ = sprite;
      *psprite++ = 0;
    }

    if (dotile || pick_move_timer[i]) {
      frame = pick_type_anim_frames[pick_anim_frame[i]];
      if (pick_move_timer[i]) {
        animdone = 0;
        *psprite++ = pick_y[i];
        *psprite++ = pick_x[i];
        *psprite++ = frame;
        *psprite++ = 0;

        pick_x[i] += pick_dx[i];
        pick_y[i] += pick_dy[i] + hopy12[--pick_move_timer[i]];

        if (pick_move_timer[i] == 0) {
          set_tile_during_vbl(pick_pos[i], frame);
        }
      } else {
        set_tile_during_vbl(pick_pos[i], frame);
      }
    }
  }

  // Loop through all mobs, update animations
  for (i = 0; i < num_mobs; ++i) {
    if (fogmap[mob_pos[i]]) { continue; }

    dosprite = dotile = 0;
    if (tile_timer == 1 && IS_ANIMATED_TILE(tmap[mob_pos[i]])) {
      dotile = 1;
    }

    if (--mob_anim_timer[i] == 0) {
      mob_anim_timer[i] = mob_anim_speed[i];
      if (++mob_anim_frame[i] == mob_type_anim_start[mob_type[i] + 1]) {
        mob_anim_frame[i] = mob_type_anim_start[mob_type[i]];
      }
      dotile = 1;
    }

    if (i + 1 == key_mob || mob_move_timer[i] || mob_flash[i]) {
      dosprite = 1;
    }

    if (dotile || dosprite) {
      frame = mob_type_anim_frames[mob_anim_frame[i]];

      if (dosprite) {
        prop = mob_flip[i] ? S_FLIPX : 0;

        *psprite++ = mob_y[i];
        *psprite++ = mob_x[i];
        if (mob_flash[i]) {
          *psprite++ = frame - TILE_MOB_FLASH_DIFF;
          *psprite++ = S_PALETTE | prop;
          --mob_flash[i];
        } else {
          *psprite++ = frame;
          *psprite++ = prop | (i + 1 == key_mob ? S_PALETTE : 0);
        }
      }

      if (mob_flip[i]) {
        frame += TILE_FLIP_DIFF;
      }

      if (mob_move_timer[i]) {
        animdone = 0;
        mob_x[i] += mob_dx[i];
        mob_y[i] += mob_dy[i];

        --mob_move_timer[i];
        if (mob_anim_state[i] == MOB_ANIM_STATE_HOP4) {
          mob_y[i] += hopy4[mob_move_timer[i]];
        }

        if (mob_move_timer[i] == 0) {
          switch (mob_anim_state[i]) {
            case MOB_ANIM_STATE_BUMP1:
              mob_anim_state[i] = MOB_ANIM_STATE_BUMP2;
              mob_move_timer[i] = BUMP_FRAMES;
              mob_dx[i] = -mob_dx[i];
              mob_dy[i] = -mob_dy[i];
              break;

            case MOB_ANIM_STATE_WALK:
              trigger_step(i);
              goto done;

            done:
            case MOB_ANIM_STATE_BUMP2:
            case MOB_ANIM_STATE_HOP4:
              mob_anim_state[i] = MOB_ANIM_STATE_NONE;
              mob_dx[i] = mob_dy[i] = 0;
              set_tile_during_vbl(mob_pos[i], frame);
              break;
          }
          // Reset animation speed
          mob_anim_speed[i] = mob_type_anim_speed[mob_type[i]];
        }
      } else if (dotile) {
        set_tile_during_vbl(mob_pos[i], frame);
      }
    }
  }

  // Draw all dead mobs as sprites
  for (i = 0; i < num_dead_mobs;) {
    if (--dmob_timer[i] == 0) {
      --num_dead_mobs;
      if (i != num_dead_mobs) {
        dmob_x[i] = dmob_x[num_dead_mobs];
        dmob_y[i] = dmob_y[num_dead_mobs];
        dmob_tile[i] = dmob_tile[num_dead_mobs];
        dmob_prop[i] = dmob_prop[num_dead_mobs];
      }
    } else {
      *psprite++ = dmob_y[i];
      *psprite++ = dmob_x[i];
      *psprite++ = dmob_tile[i];
      *psprite++ = dmob_prop[i];
      ++i;
    }
  }

  next_sprite = (OAM_item_t*)psprite;

  if (animdone) {
    pass_turn();
  }
}

void set_tile_during_vbl(u8 pos, u8 tile) {
  u16 addr = POS_TO_ADDR(pos);
  u8 *ptr = mob_tile_code_ptr;
  if ((addr >> 8) != (mob_last_tile_addr >> 8)) {
    *ptr++ = 0x21; // ld hl, addr
    *ptr++ = (u8)addr;
    *ptr++ = addr >> 8;
  } else {
    *ptr++ = 0x2e; // ld l, lo(addr)
    *ptr++ = (u8)addr;
  }
  mob_last_tile_addr = addr;
  if (tile != mob_last_tile_val) {
    *ptr++ = 0x3e; // ld a, tile
    *ptr++ = tile;
    mob_last_tile_val = tile;
  }
  *ptr++ = 0x77; // ld (hl), a
  mob_tile_code_ptr = ptr;
}

void set_digit_tile_during_vbl(u16 addr, u8 value) {
  u8 *ptr = mob_tile_code_ptr;
  *ptr++ = 0x21; // ld hl, addr
  *ptr++ = addr & 0xff;
  *ptr++ = addr >> 8;
  *ptr++ = 0x3e; // ld a, tile
  *ptr++ = mob_last_tile_val = TILE_0 + value;
  *ptr++ = 0x77; // ld (hl), a
  mob_last_tile_addr = INV_HP_ADDR;
  mob_tile_code_ptr = ptr;
}

void set_tile_range_during_vbl(u16 addr, u8* src, u8 len) {
  u8 *dst = mob_tile_code_ptr;
  *dst++ = 0x21; // ld hl, addr
  *dst++ = addr & 0xff;
  *dst++ = addr >> 8;
  do {
    *dst++ = 0x3e; // ld a, tile
    *dst++ = *src++;
    *dst++ = 0x22; // ld (hl+), a
  } while(--len);
  mob_last_tile_addr = addr + len;
  mob_last_tile_val = *--src;
  mob_tile_code_ptr = dst;
}

void update_tile(u8 pos, u8 tile) {
  set_tile_during_vbl(pos, dtmap[pos] = tmap[pos] = tile);
}

void update_tile_if_unfogged(u8 pos, u8 tile) {
  // If the tile is already unfogged then updated it, otherwise just
  // change tmap.
  if (fogmap[pos]) {
    tmap[pos] = tile;
  } else {
    update_tile(pos, tile);
  }
}

void unfog_tile(u8 pos) {
  set_tile_during_vbl(pos, dtmap[pos] = tmap[pos]);
}

void trigger_step(u8 mob) {
  u8 pos, tile, ptype, pindex, i, flt_start, flt_end, x, y, len, teleported;
  u16 equip_addr;

  teleported = 0;
redo:

  pos = mob_pos[mob];
  tile = tmap[pos];

  if (mob == PLAYER_MOB) {
    dosight = 1;

    if (tile == TILE_END) {
      donextlevel = 1;
      recover = 1; // Recover blindness on the next floor
    }

    // Handle pickups
    if ((pindex = pickmap[pos])) {
      ptype = pick_type[--pindex];
      if (ptype == PICKUP_TYPE_HEART) {
        if (mob_hp[PLAYER_MOB] < 9) {
          ++mob_hp[PLAYER_MOB];
          set_digit_tile_during_vbl(INV_HP_ADDR, mob_hp[PLAYER_MOB]);
        }
      } else if (ptype == PICKUP_TYPE_KEY) {
        if (num_keys < 9) {
          ++num_keys;
          set_digit_tile_during_vbl(INV_KEYS_ADDR, num_keys);
        }
      } else {
        // Is there a free spot in the equip?
        equip_addr = INV_EQUIP_ADDR;
        len = pick_type_name_start[ptype + 1] - pick_type_name_start[ptype];
        for (i = 0; i < MAX_EQUIPS; ++i) {
          if (equip_type[i] == PICKUP_TYPE_NONE) {
            // Use this slot
            equip_type[i] = ptype;
            equip_charge[i] = PICKUP_CHARGES;
            set_tile_range_during_vbl(
                equip_addr, pick_type_name_tile + pick_type_name_start[ptype],
                len);

            // Update the inventory message if the selection was set to this
            // equip slot
            inv_msg_update |= inv_select == i;
            goto pickup;
          } else if (equip_type[i] == ptype) {
            // Increase charges
            if (equip_charge[i] < 9) {
              equip_charge[i] += PICKUP_CHARGES;
              if (equip_charge[i] > 9) { equip_charge[i] = 9; }
              set_digit_tile_during_vbl(equip_addr + len - 2, equip_charge[i]);
            }
            goto pickup;
          }
          equip_addr += 32;
        }
        // No room, display "full!"
        ptype = PICKUP_TYPE_FULL;
        goto done;
      }

    pickup:
      delpick(pindex);

    done:
      flt_start = float_pick_type_start[ptype];
      flt_end = float_pick_type_start[ptype + 1];
      x = POS_TO_X(pos) + float_pick_type_x_offset[ptype];
      y = POS_TO_Y(pos);

      for (i = flt_start; i < flt_end; ++i) {
        shadow_OAM[next_float].x = x;
        shadow_OAM[next_float].y = y;
        shadow_OAM[next_float].tile = float_pick_type_tiles[i];
        float_time[next_float - BASE_FLOAT] = FLOAT_FRAMES;
        ++next_float;
        x += 8;
      }
    }

    // Expose the exit button for this void region.
    u8 room;
    if ((room = roommap[pos]) >= num_rooms) {
      room -= num_rooms + 1;
      u8 exitpos = void_exit[room];
      u8 exittile = tmap[exitpos];
      if (TILE_HAS_CRACKED_VARIANT(exittile)) {
        exittile += TILE_VOID_EXIT_DIFF;
        update_tile_if_unfogged(exitpos, exittile);
      }
    }
  }

  // Step on saw
  if (tile == TILE_SAW) {
    u8 hp = mob_hp[mob];
    hitmob(mob, 3);
    if (mob != PLAYER_MOB && hp < 3) {
      droppick_rnd(pos);
    }
    update_tile(pos, TILE_SAW_BROKEN);
    nop_saw_anim(pos);
  } else if (tile == TILE_TELEPORTER && !teleported) {
    u8 newpos = 0;
    do {
      if (tmap[newpos] == TILE_TELEPORTER && !IS_MOB(newpos)) {
        break;
      }
    } while(++newpos);

    if (pos != newpos) {
      // Move mob position in mobmap
      mobmap[pos] = 0;
      mobmap[newpos] = mob + 1;
      mob_pos[mob] = newpos;

      // Reset mob anim timer so the tile is updated next frame
      mob_anim_frame[mob] = 1;

      teleported = 1;
      goto redo;
    }
  }
}

void vbl_interrupt(void) {
  if (doupdatemap) {
    // update floor number
    *(u8 *)(INV_FLOOR_ADDR) = floor_tile[0];
    *(u8 *)(INV_FLOOR_ADDR + 1) = floor_tile[1];
    set_bkg_tiles(MAP_X_OFFSET, MAP_Y_OFFSET, MAP_WIDTH, MAP_HEIGHT, dtmap);
    doupdatemap = 0;
  }
  if (--tile_timer == 0) {
    tile_timer = TILE_ANIM_FRAMES;
    tile_inc ^= TILE_ANIM_FRAME_DIFF;
    ((vfp)tile_code)();
  }

  ((vfp)mob_tile_code)();
}

void addfloat(u8 pos, u8 tile) {
  shadow_OAM[next_float].x = POS_TO_X(pos);
  shadow_OAM[next_float].y = POS_TO_Y(pos);
  shadow_OAM[next_float].tile = tile;
  float_time[next_float - BASE_FLOAT] = FLOAT_FRAMES;
  ++next_float;
}

void showmsg(u8 index, u8 y) {
  u8 i, j;
  u8* p = (u8*)shadow_OAM;
  for (j = 0; j < 2; ++j) {
    for (i = 0; i < 9; ++i) {
      *p++ = y;
      *p++ = 52 + (i << 3);
      *p++ = msg_tiles[index++];
      *p++ = 0;
    }
    y += 8;
  }
  msg_timer = MSG_FRAMES;
}

void update_floats_and_msg_and_sprs(void) {
  u8 i, j;
  // Hide message if timer runs out
  if (msg_timer && --msg_timer == 0) {
    for (i = 0; i < MSG_LEN; ++i) {
      hide_sprite(i);
    }
  }

  // Draw float sprites and remove them if timed out
  for (i = BASE_FLOAT; i < next_float;) {
    j = i - BASE_FLOAT;
    --float_time[j];
    if (float_time[j] == 0) {
      --next_float;
      if (next_float != BASE_FLOAT) {
        shadow_OAM[i].x = shadow_OAM[next_float].x;
        shadow_OAM[i].y = shadow_OAM[next_float].y;
        shadow_OAM[i].tile = shadow_OAM[next_float].tile;
        float_time[j] = float_time[next_float - BASE_FLOAT];
        hide_sprite(next_float);
      } else {
        hide_sprite(i);
      }
      continue;
    } else {
      shadow_OAM[i].y -= float_diff_y[float_time[j]];
    }
    ++i;
  }

  // Draw sprs using the same ones allocated for messages
  for (i = 0; i < num_sprs;) {
    if (--spr_timer[i] == 0) {
      if (--num_sprs != i) {
        spr_type[i] = spr_type[num_sprs];
        spr_anim_frame[i] = spr_anim_frame[num_sprs];
        spr_anim_timer[i] = spr_anim_timer[num_sprs];
        spr_anim_speed[i] = spr_anim_speed[num_sprs];
        spr_x[i] = spr_x[num_sprs];
        spr_y[i] = spr_y[num_sprs];
        spr_dx[i] = spr_dx[num_sprs];
        spr_dy[i] = spr_dy[num_sprs];
        spr_drag[i] = spr_drag[num_sprs];
        spr_timer[i] = spr_timer[num_sprs];
      }
      hide_sprite(num_sprs);
    } else {
      if (--spr_anim_timer[i] == 0) {
        ++spr_anim_frame[i];
        spr_anim_timer[i] = spr_anim_speed[i];
      }
      spr_x[i] += spr_dx[i];
      spr_y[i] += spr_dy[i];
      if (spr_drag[i]) {
        spr_dx[i] = drag(spr_dx[i]);
        spr_dy[i] = drag(spr_dy[i]);
      }
      shadow_OAM[i].x = spr_x[i] >> 8;
      shadow_OAM[i].y = spr_y[i] >> 8;
      shadow_OAM[i].tile = spr_anim_frame[i];
      shadow_OAM[i].prop = 0;
      ++i;
    }
  }
}

void inv_animate(void) {
  if (inv_anim_timer) {
    --inv_anim_timer;
    WY_REG = inv_anim_up ? inventory_up_y[inv_anim_timer]
                         : inventory_down_y[inv_anim_timer];
  }

  if (inv_anim_timer || inv_anim_up) {
    if (--inv_select_timer == 0) {
      inv_select_timer = INV_SELECT_FRAMES;
      ++inv_select_frame;
    }
    next_sprite->y = WY_REG + INV_SELECT_Y_OFFSET + (inv_select << 3);
    next_sprite->x = INV_SELECT_X_OFFSET + pickbounce[inv_select_frame & 7];
    next_sprite->tile = 0;
    next_sprite->prop = 0;
    ++next_sprite;

    if (inv_msg_update) {
      inv_msg_update = 0;

      u16 addr = INV_DESC_ADDR;
      u8 pick = equip_type[inv_select];
      u8 i;
      for (i = 0; i < NUM_INV_ROWS; ++i) {
        u8 len = pick_type_desc_len[pick][i];
        if (len) {
          set_tile_range_during_vbl(
              addr, pick_type_desc_tile + pick_type_desc_start[pick][i], len);
        } else {
          *mob_tile_code_ptr++ = 0x21; // ld hl, addr
          *mob_tile_code_ptr++ = addr & 0xff;
          *mob_tile_code_ptr++ = addr >> 8;
        }

        // Fill the rest of the row with 0
        u8* dst = mob_tile_code_ptr;
        *dst++ = 0xaf; // xor a
        while (len < INV_ROW_LEN) {
          *dst++ = 0x22; // ld (hl+), a
          ++len;
        }
        mob_last_tile_addr = addr + INV_ROW_LEN;
        mob_last_tile_val = 0;
        mob_tile_code_ptr = dst;
        addr += 32;
      }
    }
  }
}

void update_obj1_pal(void) {
  if (--obj_pal1_timer == 0) {
    obj_pal1_timer = OBJ1_PAL_FRAMES;
    OBP1_REG = obj_pal1[obj_pal1_index = (obj_pal1_index + 1) & 7];
  }
}

void fadeout(void) {
  u8 i, j;
  i = 3;
  do {
    for (j = 0; j < FADE_FRAMES; ++j) { wait_vbl_done(); }
    BGP_REG = OBP0_REG = OBP1_REG = fadepal[i];
  } while(i--);
}

void fadein(void) {
  u8 i, j;
  i = 0;
  do {
    for (j = 0; j < FADE_FRAMES; ++j) { wait_vbl_done(); }
    BGP_REG = OBP0_REG = OBP1_REG = fadepal[i];
  } while(++i != 4);
}

void addmob(MobType type, u8 pos) {
  mob_type[num_mobs] = type;
  mob_anim_frame[num_mobs] = mob_type_anim_start[type];
  mob_anim_timer[num_mobs] = 1;
  mob_anim_speed[num_mobs] = mob_type_anim_speed[type];
  mob_pos[num_mobs] = pos;
  mob_x[num_mobs] = POS_TO_X(pos);
  mob_y[num_mobs] = POS_TO_Y(pos);
  mob_move_timer[num_mobs] = 0;
  mob_anim_state[num_mobs] = MOB_ANIM_STATE_NONE;
  mob_flip[num_mobs] = 0;
  mob_task[num_mobs] = mob_type_ai_wait[type];
  mob_target_pos[num_mobs] = 0;
  mob_ai_cool[num_mobs] = 0;
  mob_active[num_mobs] = 0;
  mob_charge[num_mobs] = 0;
  mob_hp[num_mobs] = mob_type_hp[type];
  ++num_mobs;
  mobmap[pos] = num_mobs; // index+1
}

void delmob(u8 index) {
  mobmap[mob_pos[index]] = 0;

  // If this mob was the key mob, then there is no more key mob.
  if (index + 1 == key_mob) {
    key_mob = 0;
  } else if (num_mobs == key_mob) {
    // If the last mob was the key mob, update it to index
    key_mob = index + 1;
  }

  --num_mobs;
  if (index != num_mobs) {
    mob_type[index] = mob_type[num_mobs];
    mob_anim_frame[index] = mob_anim_frame[num_mobs];
    mob_anim_timer[index] = mob_anim_timer[num_mobs];
    mob_anim_speed[index] = mob_anim_speed[num_mobs];
    mob_pos[index] = mob_pos[num_mobs];
    mob_x[index] = mob_x[num_mobs];
    mob_y[index] = mob_y[num_mobs];
    mob_dx[index] = mob_dx[num_mobs];
    mob_dy[index] = mob_dy[num_mobs];
    mob_move_timer[index] = mob_move_timer[num_mobs];
    mob_anim_state[index] = mob_anim_state[num_mobs];
    mob_flip[index] = mob_flip[num_mobs];
    mob_task[index] = mob_task[num_mobs];
    mob_target_pos[index] = mob_target_pos[num_mobs];
    mob_ai_cool[index] = mob_ai_cool[num_mobs];
    mob_active[index] = mob_active[num_mobs];
    mob_charge[index] = mob_charge[num_mobs];
    mob_hp[index] = mob_hp[num_mobs];
    mobmap[mob_pos[index]] = index + 1;
  }
}

void addpick(PickupType type, u8 pos) {
  pick_type[num_picks] = type;
  pick_anim_frame[num_picks] = pick_type_anim_start[type];
  pick_anim_timer[num_picks] = 1;
  pick_pos[num_picks] = pos;
  pick_move_timer[num_picks] = 0;
  ++num_picks;
  pickmap[pos] = num_picks; // index+1
}

void delpick(u8 index) {
  pickmap[pick_pos[index]] = 0;
  --num_picks;
  if (index != num_picks) {
    pick_type[index] = pick_type[num_picks];
    pick_anim_frame[index] = pick_anim_frame[num_picks];
    pick_anim_timer[index] = pick_anim_timer[num_picks];
    pick_pos[index] = pick_pos[num_picks];
    pick_x[index] = pick_x[num_picks];
    pick_y[index] = pick_y[num_picks];
    pick_dx[index] = pick_dx[num_picks];
    pick_dy[index] = pick_dy[num_picks];
    pick_move_timer[index] = pick_move_timer[num_picks];
    pickmap[pick_pos[index]] = index + 1;
  }
}

u8 dropspot(u8 pos) {
  u8 i = 0, newpos;
  do {
    if ((newpos = is_valid(pos, drop_diff[i])) &&
        !(IS_SMARTMOB(tmap[newpos], newpos) || pickmap[newpos])) {
      return newpos;
    }
  } while (++i);
  return 0;
}

void droppick(PickupType type, u8 pos) {
  u8 droppos = dropspot(pos);
  addpick(type, pos);
  pickhop(num_picks - 1, droppos);
}

void droppick_rnd(u8 pos) {
  droppick(randint(10) + 2, pos);
}

void adddeadmob(u8 index) {
  dmob_x[num_dead_mobs] = mob_x[index];
  dmob_y[num_dead_mobs] = mob_y[index];
  dmob_tile[num_dead_mobs] =
      mob_type_anim_frames[mob_anim_frame[index]] - TILE_MOB_FLASH_DIFF;
  dmob_prop[num_dead_mobs] = S_PALETTE | (mob_flip[index] ? S_FLIPX : 0);
  dmob_timer[num_dead_mobs] = DEAD_MOB_FRAMES;
  ++num_dead_mobs;
}

void initdtmap(void) {
  u8 pos = 0;
  do {
    dtmap[pos] = fogmap[pos] ? 0 : tmap[pos];
  } while(++pos);
}

void mapgen(void) {
  u8 fog;
  num_picks = 0;
  num_mobs = 1;
  num_dead_mobs = 0;
  steps = 0;
  mapgeninit();
  begin_tile_anim();
  if (floor == 0) {
    fog = 0;
    copymap(0);
    addmob(MOB_TYPE_CHEST, 119);  // x=7,y=7
  } else if (floor == 10) {
    fog = 0;
    copymap(1);
  } else {
    fog = 1;
    roomgen();
    mazeworms();
    carvedoors();
    carvecuts();
    startend();
    fillends();
    prettywalls();
    voids();
    chests();
    decoration();
    spawnmobs();
  }
  end_tile_anim();
  memset(fogmap, fog, sizeof(fogmap));
  sight();
  initdtmap();
}

void mapgeninit(void) {
  memset(roommap, 0, sizeof(roommap));
  memset(mobmap, 0, sizeof(mobmap));
  memset(pickmap, 0, sizeof(pickmap));
  memset(sigmap, 255, sizeof(sigmap));
  memset(ds_sets, 0, sizeof(ds_sets));
  memset(sawmap, 0, sizeof(sawmap));

  dogate = 0;

  // Initialize disjoint set
  ds_nextid = 0;
  ds_parents[0] = 0;
  ds_num_sets = 0;

  num_rooms = 0;
  num_voids = 0;
  start_room = 0;

  mob_anim_timer[PLAYER_MOB] = 1;
  mob_anim_speed[num_mobs] = mob_type_anim_speed[MOB_TYPE_PLAYER];
  mob_move_timer[PLAYER_MOB] = 0;
  mob_flip[PLAYER_MOB] = 0;
}

void roomgen(void) {
  u8 failmax = 5, maxwidth = 10, maxheight = 10, maxh;
  u8 sig, valid, walkable, val, w, h, pos;

  dogate = floor == 3 || floor == 6 || floor == 9;

  if (dogate) {
    // Pick a random gate map
    copymap(2 + randint(NUM_GATE_MAPS));
  } else {
    // Start with a completely empty map (all walls)
    memset(tmap, TILE_WALL, sizeof(tmap));
  }

  do {
    // Pick width and height for room.
    w = 3 + randint(maxwidth - 2);
    maxh = other_dim[w - 3];
    if (maxh > maxheight) maxh = maxheight;
    h = 3 + randint(maxh - 2);

    // Update width/height maps
    //
    // The basic idea is to start in the lower-right corner and work backward.
    // There are two maps; one that tracks whether a room of this width will
    // fit, and the other tracks whether a room of this height will fit. When
    // both values are 0, the room fits.
    //
    // The room must have a one tile border around it, except for when it is
    // adjacent to the map edge.
    num_cands = 0;
    pos = 255;
    do {
      valid = validmap[pos];
      sig = ~sigmap[pos];
      walkable = !IS_WALL_TILE(tmap[pos]);
      if (walkable || (sig & VALID_L_OR_UL)) {
        // This tile is either in a room, or on the right or lower border of
        // the room.
        val = h + 1;
      } else if (valid & VALID_D) {
        // This tile is above a room or the bottom edge; we subtract one from
        // the value below to track whether it is far enough away to fit a room
        // of height `h`.
        val = tempmap[POS_D(pos)];
        if (val) { --val; }
      } else {
        // This tile is on the bottom edge of the map.
        val = h - 1;
      }
      tempmap[pos] = val;

      if (walkable || (sig & VALID_U_OR_UL)) {
        // This tile is either in a room, or on the right or lower border of
        // the room.
        val = w + 1;
      } else if (valid & VALID_R) {
        val = distmap[POS_R(pos)];
        if (val) {
          // This tile is to the left of a room or the right edge of the map.
          // We also update the "height" map in this case to extend out the
          // invalid region vertically. For example, if placing a 3x3 room,
          // where there is an existing room (marked with x):
          //
          // width map   height map   overlap
          // 000000012   000000000    0000000..
          // 000000012   011112220    0........
          // 000000012   022222220    0........
          // 000000012   033333330    0........
          // 0123xxxx2   0444xxxx0    0........
          // 0123xxxx2   0444xxxx0    0........
          // 0123xxxx2   1111xxxx1    .........
          // 000000012   222222222    .........
          //
          --val;
          tempmap[pos] = tempmap[POS_R(pos)];
        } else {
          val = 0;
          if (!tempmap[pos]) {
            // This is valid position, add it to the candidate list.
            cands[num_cands++] = pos;
          }
        }
      } else {
        val = w - 1;
      }
      distmap[pos] = val;
    } while(pos--);

    if (num_cands) {
      pos = cands[randint(num_cands)];

      room_pos[num_rooms] = pos;
      room_w[num_rooms] = w;
      room_h[num_rooms] = h;
      room_avoid[num_rooms] = 0;

      // Fill room tiles at x, y
      mapset(tmap + pos, w, h, 1);
      mapset(roommap + pos, w, h, num_rooms + 1);
      sigrect_empty(pos, w, h);

      // Update disjoint set regions; we know that they're disjoint so they
      // each get their own region id (i.e. the room number)
      mapset(ds_sets + pos, w, h, ++ds_nextid);
      ds_parents[ds_nextid] = ds_nextid;
      ds_sizes[ds_nextid] = w * h; // XXX multiply
      ++ds_num_sets;

      if (++num_rooms == 1) {
        maxwidth >>= 1;
        maxheight >>= 1;
      }
    } else {
      // cannot place room
      --failmax;
    }
  } while (failmax && num_rooms < MAX_ROOMS);
}

void copymap(u8 index) {
  const u8 *src;
  u8 *dst;
  u8 pos, valid, i, w, h;

  // Fill map with default tiles. TILE_NONE is used for floor0 and floor10, and
  // TILE_WALL is used for all gate maps
  memset(tmap, index < 2 ? TILE_NONE : TILE_WALL, 256);

  // Each premade map only has a small rectangular region to copy (map_w by
  // map_h tiles), and is offset by map_pos.
  src = map_bin + map_start[index];
  dst = tmap + map_pos[index];
  w = map_w[index];
  h = map_h[index];
  do {
    i = w;
    do {
      *dst++ = *src++;
    } while (--i);
    dst += MAP_WIDTH - w;
  } while (--h);

  // Update signature map for empty tiles
  pos = 0;
  do {
    switch (tmap[pos]) {
      case TILE_END:
        endpos = pos;
        goto empty;

      case TILE_START:
      case TILE_ENTRANCE:
        startpos = pos;
        mobmap[mob_pos[PLAYER_MOB]] = 0; // Clear old player location
        mob_pos[PLAYER_MOB] = pos;
        mobmap[pos] = PLAYER_MOB + 1;
        goto empty;

      case TILE_GATE:
        // Temporarily change to stairs so the empty spaces behind the gate
        // can be part of the spanning graph created during the carvedoors()
        // step.
        tmap[pos] = TILE_END;
        gatepos = pos;
        goto empty;

      case TILE_TORCH_LEFT:
      case TILE_TORCH_RIGHT:
        add_tile_anim(pos, tmap[pos]);
        goto empty;

      case TILE_HEART:
        heartpos = pos;
        tmap[pos] = TILE_RUG;
        goto empty;

      empty:
      case TILE_CARPET:
      case TILE_EMPTY:
      case TILE_STEPS:
        // Update sigmap
        sigempty(pos);

        // Update disjoint set for the gate area
        ++ds_nextid;
        ds_sets[pos] = ds_nextid;
        ds_parents[ds_nextid] = ds_nextid;
        ds_sizes[ds_nextid] = 1;

        valid = validmap[pos];
        if (valid & VALID_U) { ds_union(ds_nextid, ds_sets[POS_U(pos)]); }
        if (valid & VALID_L) { ds_union(ds_nextid, ds_sets[POS_L(pos)]); }
        if (valid & VALID_R) { ds_union(ds_nextid, ds_sets[POS_R(pos)]); }
        if (valid & VALID_D) { ds_union(ds_nextid, ds_sets[POS_D(pos)]); }
        break;
    }
  } while (++pos);
}

void mazeworms(void) {
  u8 pos, cand, valid;

  memset(tempmap, 0, sizeof(tempmap));

  // Find all candidates for worm start position
  num_cands = 0;
  pos = 0;
  do {
    update_carve1(pos);
  } while(++pos);

  do {
    if (num_cands) {
      // Choose a random start, and calculate its offset
      cand = randint(num_cands);
      pos = 0;
      do {
        if (tempmap[pos] == 2 && cand-- == 0) { break; }
      } while(++pos);

      // Update the disjoint set with this new region.
      ++ds_nextid;
      ++ds_num_sets;
      ds_parents[ds_nextid] = ds_nextid;
      ds_sizes[ds_nextid] = 0;

      digworm(pos);

      // Only the first spot may be connected to another empty tile. No other
      // tile can be connect to another region (since it would not be
      // carvable). So we only need to merge regions here.
      valid = validmap[pos];
      if (valid & VALID_U) { ds_union(ds_nextid, ds_sets[POS_U(pos)]); }
      if (valid & VALID_L) { ds_union(ds_nextid, ds_sets[POS_L(pos)]); }
      if (valid & VALID_R) { ds_union(ds_nextid, ds_sets[POS_R(pos)]); }
      if (valid & VALID_D) { ds_union(ds_nextid, ds_sets[POS_D(pos)]); }
    }
  } while(num_cands > 1);
}

void update_carve1(u8 pos) {
  u8 tile = tmap[pos];
  u8 result;
  if (tile != TILE_FIXED_WALL && IS_WALL_TILE(tile) && CAN_CARVE(pos)) {
    result = 1;
    if (!nexttoroom4(pos, validmap[pos])) {
      ++result;
      if (tempmap[pos] != 2) { ++num_cands; }
    }
  } else {
    result = 0;
    if (tempmap[pos] == 2) { --num_cands; }
  }
  tempmap[pos] = result;
}

u8 nexttoroom4(u8 pos, u8 valid) {
  return ((valid & VALID_U) && roommap[POS_U(pos)]) ||
         ((valid & VALID_L) && roommap[POS_L(pos)]) ||
         ((valid & VALID_R) && roommap[POS_R(pos)]) ||
         ((valid & VALID_D) && roommap[POS_D(pos)]);
}

u8 nexttoroom8(u8 pos, u8 valid) {
  return nexttoroom4(pos, valid) ||
         ((valid & VALID_UL) && roommap[POS_UL(pos)]) ||
         ((valid & VALID_UR) && roommap[POS_UR(pos)]) ||
         ((valid & VALID_DL) && roommap[POS_DL(pos)]) ||
         ((valid & VALID_DR) && roommap[POS_DR(pos)]);
}

void digworm(u8 pos) {
  u8 dir, step, num_dirs, valid;

  // Pick a random direction
  dir = URAND() & 3;
  step = 0;
  do {
    tmap[pos] = 1; // Set tile to empty.
    sigempty(pos); // Update neighbor signatures.

    // Remove this tile from the carvable map
    if (tempmap[pos] == 2) { --num_cands; }
    tempmap[pos] = 0;

    // Update neighbors
    valid = validmap[pos];
    if (valid & VALID_UL) { update_carve1(POS_UL(pos)); }
    if (valid & VALID_U)  { update_carve1(POS_U (pos)); }
    if (valid & VALID_UR) { update_carve1(POS_UR(pos)); }
    if (valid & VALID_L)  { update_carve1(POS_L (pos)); }
    if (valid & VALID_R)  { update_carve1(POS_R (pos)); }
    if (valid & VALID_DL) { update_carve1(POS_DL(pos)); }
    if (valid & VALID_D)  { update_carve1(POS_D (pos)); }
    if (valid & VALID_DR) { update_carve1(POS_DR(pos)); }

    // Update the disjoint set
    ds_sets[pos] = ds_nextid;
    ++ds_sizes[ds_nextid];

    // Continue in the current direction, unless we can't carve. Also randomly
    // change direction after 3 or more steps.
    if (!((valid & dirvalid[dir]) && tempmap[POS_DIR(pos, dir)]) ||
        (URAND_50_PERCENT() && step > 2)) {
      step = 0;
      num_dirs = 0;
      if ((valid & VALID_L) && tempmap[POS_L(pos)]) { cands[num_dirs++] = 0; }
      if ((valid & VALID_R) && tempmap[POS_R(pos)]) { cands[num_dirs++] = 1; }
      if ((valid & VALID_U) && tempmap[POS_U(pos)]) { cands[num_dirs++] = 2; }
      if ((valid & VALID_D) && tempmap[POS_D(pos)]) { cands[num_dirs++] = 3; }
      if (num_dirs == 0) return;
      dir = cands[randint(num_dirs)];
    }
    pos = POS_DIR(pos, dir);
    ++step;
  } while(1);
}

void carvedoors(void) {
  u8 pos, match, diff;

  memset(tempmap, 0, sizeof(tempmap));

  pos = 0;
  num_cands = 0;
  do {
    update_door1(pos);
  } while(++pos);

  do {
    // Pick a random door, and calculate its position.
    pos = getpos(randint(num_cands));

    // Merge the regions, if possible. They may be already part of the same
    // set, in which case they should not be joined.
    match = IS_DOOR(pos);
    diff = match == 1 ? 16 : 1; // 1: horizontal, 2: vertical
    if (ds_union(ds_sets[pos - diff], ds_sets[pos + diff])) {
      // Insert an empty tile to connect the regions
      ds_sets[pos] = ds_sets[pos + diff];
      tmap[pos] = 1;
      sigempty(pos); // Update neighbor signatures.

      // Update neighbors
      u8 valid = validmap[pos];
      if (valid & VALID_U) { update_door1(POS_U(pos)); }
      if (valid & VALID_L) { update_door1(POS_L(pos)); }
      if (valid & VALID_R) { update_door1(POS_R(pos)); }
      if (valid & VALID_D) { update_door1(POS_D(pos)); }
    }

    // Remove this tile from the carvable map
    --num_cands;
    tempmap[pos] = 0;
  } while (num_cands);

  // TODO: its possible to generate a map that is not fully-connected; in that
  // case we need to start over.
}

void update_door1(u8 pos) {
  // A door connects two regions in the following patterns:
  //   * 0 *   * 1 *
  //   1   1   0   0
  //   * 0 *   * 1 *
  //
  // Where 1s are walls and 0s are not. The `match` index returns whether it is
  // a horizontal door or vertical. A door is only allowed between two regions
  // that are not already connected. We use the disjoint set to determine this
  // quickly below.
  u8 tile = tmap[pos];
  u8 result = tile != TILE_FIXED_WALL && IS_WALL_TILE(tile) && IS_DOOR(pos);
  if (tempmap[pos] != result) {
    if (result) {
      ++num_cands;
    } else {
      --num_cands;
    }
    tempmap[pos] = result;
  }
}

void carvecuts(void) {
  u8 pos, diff, match;
  u8 maxcuts = 3;

  num_cands = 0;
  pos = 0;
  do {
    update_door1(pos);
  } while(++pos);

  do {
    // Pick a random cut, and calculate its position.
    pos = getpos(randint(num_cands));

    // Calculate distance from one side of the door to the other.
    match = IS_DOOR(pos);
    diff = match == 1 ? 16 : 1; // 1: horizontal, 2: vertical
    // TODO: we only need the distance to the other size of the door; use a
    // faster routine for this?
    calcdist(pos + diff);

    // Remove this candidate
    tempmap[pos] = 0;
    --num_cands;

    // Connect the regions (creating a cycle) if the distance between the two
    // sides is > 20 steps (21 is used because the distance starts at 1; that's
    // so that 0 can be used to represent an unvisited cell).
    if (distmap[(u8)(pos - diff)] > 21) {
      tmap[pos] = 1;
      sigempty(pos); // Update neighbor signatures.
      --maxcuts;
    }
  } while (num_cands && maxcuts);
}

void calcdist(u8 pos) {
  u8 head, oldtail, newtail, sig, valid, dist, newpos;
  cands[head = 0] = pos;
  newtail = 1;

  memset(distmap, 0, sizeof(distmap));
  distmap[pos] = dist = 1;

  do {
    oldtail = newtail;
    ++dist;
    do {
      pos = cands[head++];
      valid = validmap[pos];
      sig = ~sigmap[pos] & valid;
      if (!distmap[newpos = POS_U(pos)]) {
        if (valid & VALID_U) { distmap[newpos] = dist; }
        if (sig & VALID_U) { cands[newtail++] = newpos; }
      }
      if (!distmap[newpos = POS_L(pos)]) {
        if (valid & VALID_L) { distmap[newpos] = dist; }
        if (sig & VALID_L) { cands[newtail++] = newpos; }
      }
      if (!distmap[newpos = POS_R(pos)]) {
        if (valid & VALID_R) { distmap[newpos] = dist; }
        if (sig & VALID_R) { cands[newtail++] = newpos; }
      }
      if (!distmap[newpos = POS_D(pos)]) {
        if (valid & VALID_D) { distmap[newpos] = dist; }
        if (sig & VALID_D) { cands[newtail++] = newpos; }
      }
    } while (head != oldtail);
  } while (oldtail != newtail);
}

void startend(void) {
  u8 pos, best, score;

  if (!dogate) {
    // Pick a random walkable location
    do {
      endpos = URAND();
    } while (IS_WALL_TILE(tmap[endpos]));
  }

  calcdist(endpos);

  startpos = 0;
  best = 0;
  // Find the furthest point from endpos.
  pos = 0;
  do {
    if (distmap[pos] > best) {
      startpos = pos;
      best = distmap[pos];
    }
  } while(++pos);

  calcdist(startpos);

  if (!dogate) {
    // Now pick the furthest, freestanding location in a room from the startpos.
    pos = 0;
    best = 0;
    do {
      if (distmap[pos] > best && roommap[pos] && IS_FREESTANDING(pos)) {
        endpos = pos;
        best = distmap[pos];
      }
    } while(++pos);
  }

  // Finally pick the closest point that has the best startscore.
  pos = 0;
  best = 255;
  do {
    score = startscore(pos);
    if (distmap[pos] && score) {
      score = distmap[pos] + score;
      if (score < best) {
        best = score;
        startpos = pos;
      }
    }
  } while(++pos);

  if (roommap[startpos]) {
    start_room = roommap[startpos];
  }

  tmap[startpos] = TILE_START;
  tmap[endpos] = TILE_END;
  sigempty(startpos);
  sigempty(endpos);
  mob_pos[PLAYER_MOB] = startpos;
  mobmap[startpos] = PLAYER_MOB + 1;

  if (dogate) {
    tmap[gatepos] = TILE_GATE;
    if (floor < 9) {
      addpick(PICKUP_TYPE_HEART, heartpos);
    }
  }
}

u8 startscore(u8 pos) {
  u8 score = IS_FREESTANDING(pos);
  // If the position is not in a room...
  if (!roommap[pos]) {
    // ...and not next to a room...
    if (!nexttoroom8(pos, validmap[pos])) {
      if (score) {
        // ...and it's freestanding, then give the best score
        return 1;
      } else if (CAN_CARVE(pos)) {
        // If the position is carvable, give a low score
        return 6;
      }
    }
  } else if (score) {
    // otherwise the position is in a room, so give a good score if it is
    // "more" freestanding (not in a corner).
    return score <= 9 ? 3 : 6;
  }
  return 0; // Invalid start position.
}

void fillends(void) {
  u8 pos, valid;
  memset(tempmap, 0, sizeof(tempmap));

  // Find all starting positions (dead ends)
  num_cands = 0;
  pos = 0;
  do {
    update_fill1(pos);
  } while(++pos);

  do {
    if (num_cands) {
      // Choose a random point, and calculate its offset
      pos = getpos(randint(num_cands));

      tmap[pos] = TILE_WALL; // Set tile to wall
      sigwall(pos);          // Update neighbor signatures.

      // Remove this cell from the dead end map
      --num_cands;
      tempmap[pos] = 0;

      valid = validmap[pos];
      if (valid & VALID_UL) { update_fill1(POS_UL(pos)); }
      if (valid & VALID_U)  { update_fill1(POS_U (pos)); }
      if (valid & VALID_UR) { update_fill1(POS_UR(pos)); }
      if (valid & VALID_L)  { update_fill1(POS_L (pos)); }
      if (valid & VALID_R)  { update_fill1(POS_R (pos)); }
      if (valid & VALID_DL) { update_fill1(POS_DL(pos)); }
      if (valid & VALID_D)  { update_fill1(POS_D (pos)); }
      if (valid & VALID_DR) { update_fill1(POS_DR(pos)); }
    }
  } while (num_cands);
}

void update_fill1(u8 pos) {
  u8 tile = tmap[pos];
  u8 result = !IS_WALL_TILE(tmap[pos]) && CAN_CARVE(pos) &&
              tile != TILE_START && tile != TILE_END;
  if (tempmap[pos] != result) {
    if (result) {
      ++num_cands;
    } else {
      --num_cands;
    }
    tempmap[pos] = result;
  }
}

void prettywalls(void) {
  u8 pos, tile;
  pos = 0;
  do {
    if (tmap[pos] == TILE_FIXED_WALL) {
      tmap[pos] = TILE_WALL;
    }
    if (tmap[pos] == TILE_WALL) {
      tile = sigmatch(pos, wallsig, wallmask);
      if (tile) {
        tile += TILE_WALLSIG_BASE;
      }
      if (TILE_HAS_CRACKED_VARIANT(tile) && URAND_12_5_PERCENT()) {
        tile += TILE_CRACKED_DIFF;
      }
      tmap[pos] = tile;
    } else if (tmap[pos] == TILE_EMPTY && (validmap[pos] & VALID_U) &&
               IS_WALL_TILE(tmap[POS_U(pos)])) {
      tmap[pos] = IS_CRACKED_WALL_TILE(tmap[POS_U(pos)])
                      ? TILE_WALL_FACE_CRACKED
                      : TILE_WALL_FACE;
    }
  } while(++pos);
}

void voids(void) {
  u8 pos, head, oldtail, newtail, valid, newpos, dir, num_walls, id, exit;

  pos = 0;
  num_voids = 0;

  memset(void_num_tiles, 0, sizeof(void_num_tiles));
  // Use the tempmap to keep track of void walls
  memset(tempmap, 0, sizeof(tempmap));

  do {
    if (!tmap[pos]) {
      // flood fill this region
      num_walls = 0;
      cands[head = 0] = pos;
      newtail = 1;
      id = num_rooms + num_voids + 1;

      do {
        oldtail = newtail;
        do {
          pos = cands[head++];
          if (!tmap[pos]) {
            roommap[pos] = id;
            ++void_num_tiles[num_voids];
            tmap[pos] = void_tiles[randint(sizeof(void_tiles))];
            valid = validmap[pos];

            for (dir = 0; dir < 4; ++dir) {
              if (valid & dirvalid[dir]) {
                if (!tmap[newpos = POS_DIR(pos, dir)]) {
                  cands[newtail++] = newpos;
                } else if (TILE_HAS_CRACKED_VARIANT(tmap[newpos])) {
                  ++num_walls;
                  tempmap[newpos] = id;
                }
              }
            }
          }
        } while(head != oldtail);
      } while (oldtail != newtail);

      // Pick a random wall to make into a void exit
      exit = randint(num_walls);
      newpos = 0;
      do {
        if (tempmap[newpos] == id && exit-- == 0) { break; }
      } while(++newpos);

      void_exit[num_voids] = newpos;

      // Make sure the void exit is not a cracked wall.
      if (IS_CRACKED_WALL_TILE(tmap[newpos])) {
        tmap[newpos] -= TILE_CRACKED_DIFF;
      }
      ++num_voids;
    }
  } while(++pos);
}

void chests(void) {
  u8 has_teleporter, room, chest_pos, i, pos, cand0, cand1;

  // Teleporter in level if level >= 5 and rnd(5)<1 (20%)
  has_teleporter = floor >= 5 && URAND_20_PERCENT();

  // Pick a random room
  room = randint(num_rooms);

  // Pick a random x,y in room (non-edge) for chest
  chest_pos;
  do {
    chest_pos = room_pos[room] + (((randint(room_h[room] - 2) + 1) << 4) |
                                  (randint(room_w[room] - 2) + 1));
  } while (tmap[chest_pos] != 1);

  // Pick a random position in each void and add it to the candidates. If
  // has_teleporter is true, then add chest_pos too.
  // The candidate indexes are stored starting at `num_voids`, so that the
  // actual positions can be stored starting at 0.
  for (i = 0; i < num_voids; ++i) {
    cands[1 + num_rooms + num_voids + i] = randint(void_num_tiles[i]);
  }

  // Convert these candidate to positions
  pos = 0;
  do {
    if (cands[num_voids + roommap[pos]]-- == 0) {
      // Subtract num_rooms so that the void regions starts at candidate 0 and
      // the rooms are not included.
      cands[(u8)(roommap[pos] - num_rooms - 1)] = pos;
    }
  } while (++pos);

  num_cands = num_voids;
  if (has_teleporter) {
    cands[num_cands++] = chest_pos;
  }

  if (num_cands > 2) {
    // Pick two random candidates and turn them into teleporters
    cand0 = randint(num_cands);
    cand1 = randint(num_cands - 1);
    if (cand1 == cand0) { ++cand1; }
    tmap[cands[cand0]] = tmap[cands[cand1]] = TILE_TELEPORTER;

    // Swap out the already used candidates
    cands[cand0] = cands[--num_cands];
    cands[cand1] = cands[--num_cands];
  }

  // Add chest position as a candidate if it wasn't already included above
  if (!has_teleporter) {
    cands[num_cands++] = chest_pos;
  }

  for (i = 0; i < num_cands; ++i) {
    addmob(floor >= 5 && URAND_20_PERCENT() ? MOB_TYPE_HEART_CHEST
                                            : MOB_TYPE_CHEST,
           cands[i]);
  }
}

void decoration(void) {
  RoomKind kind = ROOM_KIND_VASE;
  const u8 (*room_perm)[MAX_ROOMS] = room_permutations + randint(24);
  u8 i, room, pos, tile, w, h, mob;
  for (i = 0; i < MAX_ROOMS; ++i) {
    room = (*room_perm)[i];
    if (room >= num_rooms) { continue; }

    pos = room_pos[room];
    h = room_h[room];
    do {
      w = room_w[room];
      do {
        tile = tmap[pos];

        switch (kind) {
          case ROOM_KIND_VASE:
            mob = vase_mobs[randint(sizeof(vase_mobs))];
            if (mob && !IS_SMARTMOB(tile, pos) && IS_FREESTANDING(pos) &&
                (h == room_h[room] || h == 1 || w == room_w[room] || w == 1)) {
              addmob(mob, pos);
            }
            break;

          case ROOM_KIND_DIRT:
            if (tile == TILE_EMPTY) {
              tmap[pos] = dirt_tiles[URAND() & 3];
            }
            break;

          case ROOM_KIND_CARPET:
            if (tile == TILE_EMPTY && h != room_h[room] && h != 1 &&
                w != room_w[room] && w != 1) {
              tmap[pos] = TILE_CARPET;
            }
            // fallthrough

          case ROOM_KIND_TORCH:
            if (tile == TILE_EMPTY && URAND_33_PERCENT() && (h & 1) &&
                IS_FREESTANDING(pos)) {
              if (w == 1) {
                tmap[pos] = TILE_TORCH_RIGHT;
              } else if (w == room_w[room]) {
                tmap[pos] = TILE_TORCH_LEFT;
              }
            }
            break;

          case ROOM_KIND_PLANT:
            if (tile == TILE_EMPTY) {
              tmap[pos] = plant_tiles[randint(3)];
            } else if (tile == TILE_WALL_FACE) {
              tmap[pos] = TILE_WALL_FACE_PLANT;
            }

            if (!IS_SMARTMOB(tile, pos) && room + 1 != start_room &&
                URAND_25_PERCENT() && no_mob_neighbors(pos) &&
                IS_FREESTANDING(pos)) {
              mob = plant_mobs[randint(sizeof(plant_mobs))];
              addmob(mob, pos);
              if (mob == MOB_TYPE_WEED) {
                room_avoid[room] = 1;
              }
            }
            break;
        }

        if (kind != ROOM_KIND_PLANT && room + 1 != start_room &&
            tmap[pos] == TILE_EMPTY && !mobmap[pos] && IS_FREESTANDING(pos) &&
            URAND_10_PERCENT()) {
          tmap[pos] = TILE_SAW;
          sigwall(pos);
        }

        ++pos;
      } while(--w);
      pos += MAP_WIDTH - room_w[room];
    } while(--h);


    // Pick a room kind for the next room
    kind = randint(NUM_ROOM_KINDS);
  }
}

void begin_tile_anim(void) {
  u16 addr = (u16)&tile_inc;
  tile_code[0] = 0xf5; // push af
  tile_code[1] = 0xc5; // push bc
  tile_code[2] = 0xfa; // ld a, (NNNN)
  tile_code[3] = (u8)addr;
  tile_code[4] = addr >> 8;
  tile_code[5] = 0x47; // ld b, a
  tile_code_ptr = tile_code + 6;
  last_tile_addr = 0;
  last_tile_val = 0;
}

void end_tile_anim(void) {
  u8 *ptr = tile_code_ptr;
  *ptr++ = 0xc1; // pop bc
  *ptr++ = 0xf1; // pop af
  *ptr = 0xc9;   // ret
}

void add_tile_anim(u8 pos, u8 tile) {
  u8* ptr = tile_code_ptr;
  u16 addr = POS_TO_ADDR(pos);
  if ((addr >> 8) != (last_tile_addr >> 8)) {
    *ptr++ = 0x21; // ld hl, NNNN
    *ptr++ = (u8)addr;
    *ptr++ = addr >> 8;
  } else {
    *ptr++ = 0x2e; // ld l, NN
    *ptr++ = (u8)addr;
  }
  last_tile_addr = addr;
  if (tile != last_tile_val) {
    *ptr++ = 0x3e; // ld a, NN
    *ptr++ = tile;
    *ptr++ = 0x80; // add b
    last_tile_val = tile;
  }
  *ptr++ = 0x77; // ld (hl), a
  tile_code_ptr = ptr;
}

void nop_saw_anim(u8 pos) {
  // Only nop out the memory write
  tile_code[sawmap[pos]] = 0;
}

void spawnmobs(void) {
  u8 pos, room, mob, mobs, i;

  pos = 0;
  num_cands = 0;
  do {
    room = roommap[pos];
    if (room && room != start_room && room <= num_rooms &&
        !IS_SMARTMOB(tmap[pos], pos) && !room_avoid[room - 1]) {
      tempmap[pos] = 1;
      ++num_cands;
    } else {
      tempmap[pos] = 0;
    }
  } while (++pos);

  mobs = mob_plan_floor[floor + 1] - mob_plan_floor[floor];

  do {
    pos = getpos(randint(num_cands));
    tempmap[pos] = 0;
    --num_cands;
    --mobs;
    addmob(mob_plan[mob_plan_floor[floor] + mobs], pos);
  } while (mobs && num_cands);

  // TODO: pack mobs into avoid rooms

  // Give a mob a key, if this is a gate floor
  if (dogate) {
    num_cands = 0;
    for (i = 0; i < num_mobs; ++i) {
      if (mob_type_key[mob_type[i]]) { ++num_cands; }
    }

    i = 0;
    mob = randint(num_cands);
    while (1) {
      if (mob_type_key[mob_type[i]] && mob-- == 0) { break; }
      ++i;
    }
    key_mob = i + 1;
  } else {
    key_mob = 0;
  }
}

u8 no_mob_neighbors(u8 pos) {
  u8 valid = validmap[pos];
  return !(((valid & VALID_U) && mobmap[POS_U(pos)]) ||
           ((valid & VALID_L) && mobmap[POS_L(pos)]) ||
           ((valid & VALID_R) && mobmap[POS_R(pos)]) ||
           ((valid & VALID_D) && mobmap[POS_D(pos)]));
}

void mapset(u8* pos, u8 w, u8 h, u8 val) {
  u8 w2 = w;
  do {
    *pos++ = val;
    if (--w2 == 0) {
      w2 = w;
      --h;
      pos += MAP_WIDTH - w;
    }
  } while (h);
}

void sigrect_empty(u8 pos, u8 w, u8 h) {
  u8 w2 = w;
  do {
    sigempty(pos);
    ++pos;

    if (--w2 == 0) {
      w2 = w;
      --h;
      pos += MAP_WIDTH - w;
    }
  } while (h);
}

void sigempty(u8 pos) {
  u8 valid = validmap[pos];
  if (valid & VALID_UL) { sigmap[POS_UL(pos)] &= MASK_DR; }
  if (valid & VALID_U)  { sigmap[POS_U (pos)] &= MASK_D; }
  if (valid & VALID_UR) { sigmap[POS_UR(pos)] &= MASK_DL; }
  if (valid & VALID_L)  { sigmap[POS_L (pos)] &= MASK_R; }
  if (valid & VALID_R)  { sigmap[POS_R (pos)] &= MASK_L; }
  if (valid & VALID_DL) { sigmap[POS_DL(pos)] &= MASK_UR; }
  if (valid & VALID_D)  { sigmap[POS_D (pos)] &= MASK_U; }
  if (valid & VALID_DR) { sigmap[POS_DR(pos)] &= MASK_UL; }
}

void sigwall(u8 pos) {
  u8 valid = validmap[pos];
  if (valid & VALID_UL) { sigmap[POS_UL(pos)] |= VALID_DR; }
  if (valid & VALID_U)  { sigmap[POS_U (pos)] |= VALID_D; }
  if (valid & VALID_UR) { sigmap[POS_UR(pos)] |= VALID_DL; }
  if (valid & VALID_L)  { sigmap[POS_L (pos)] |= VALID_R; }
  if (valid & VALID_R)  { sigmap[POS_R (pos)] |= VALID_L; }
  if (valid & VALID_DL) { sigmap[POS_DL(pos)] |= VALID_UR; }
  if (valid & VALID_D)  { sigmap[POS_D (pos)] |= VALID_U; }
  if (valid & VALID_DR) { sigmap[POS_DR(pos)] |= VALID_UL; }
}

u8 sigmatch(u16 pos, u8* sig, u8* mask) {
  u8 result = 0;
  u8 val = sigmap[pos];
  while (1) {
    if (*sig == 0) { return 0; }
    if ((val | *mask) == *sig) {
      return result + 1;
    }
    ++mask;
    ++sig;
    ++result;
  }
}

u8 ds_union(u8 x, u8 y) {
  if (!y) return 0;
  if (!x) return 0;  // TODO: remove this since x is always called w/ valid id
  x = ds_find(x);
  y = ds_find(y);

  if (x == y) return 0;

  if (ds_sizes[x] < ds_sizes[y]) {
    u8 t = x;
    x = y;
    y = t;
  }

  ds_parents[y] = x;
  ds_sizes[x] = ds_sizes[x] + ds_sizes[y];
  --ds_num_sets;
  return 1;
}

u8 ds_find(u8 x) {
  // Use path-halving as described in
  // https://en.wikipedia.org/wiki/Disjoint-set_data_structure
  while (x != ds_parents[x]) {
    x = ds_parents[x] = ds_parents[ds_parents[x]];
  }
  return x;
}

u8 getpos(u8 cand) {
  u8 pos = 0;
  do {
    if (tempmap[pos] && cand-- == 0) { break; }
  } while (++pos);
  return pos;
}

const u8 randmask[] = {
  0,0,
  1,
  3,3,
  7,7,7,7,
  15,15,15,15,15,15,15,15,
  31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,
  63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,
  63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,
  127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,
  127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,
  127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,
  127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
};

u8 randint(u8 mx) {
  if (mx == 0) { return 0; }
  u8 result;
  do {
    result = URAND() & randmask[mx];
  } while (result >= mx);
  return result;
}

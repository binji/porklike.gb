#include <gb/gb.h>
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
#define MAX_MOBS 32  /* XXX Figure out what this should be */
#define MAX_FLOATS 8 /* For now; we'll probably want more */

// TODO: how big to make these arrays?
#define TILE_CODE_SIZE 128
#define MOB_TILE_CODE_SIZE 256

#define PLAYER_MOB 0

#define MAP_WIDTH 16
#define MAP_HEIGHT 16

#define TILE_ANIM_SPEED 8
#define TILE_ANIM_FRAME_DIFF 16
#define TILE_FLIP_DIFF 0x22

#define FADE_TIME 8

#define WALK_TIME 8
#define BUMP_TIME 4
#define HOP_TIME 8
#define FLOAT_TIME 70
#define QUEEN_CHARGE_TIME 2

#define AI_COOL_TIME 8

#define IS_WALL_TILE(tile) (flags_bin[tile] & 1)
#define IS_SPECIAL_TILE(tile) (flags_bin[tile] & 2)
#define IS_WALL_OR_SPECIAL_TILE(tile) (flags_bin[tile] & 3)
#define IS_OPAQUE_TILE(tile) (flags_bin[tile] & 4)
#define TILE_HAS_CRACKED_VARIANT(tile) (flags_bin[tile] & 0b01000000)
#define IS_CRACKED_WALL_TILE(tile) (flags_bin[tile] & 0b00001000)
#define IS_ANIMATED_TILE(tile) (flags_bin[tile] & 0b00010000)

#define IS_DOOR(pos) sigmatch((pos), doorsig, doormask)
#define CAN_CARVE(pos) sigmatch((pos), carvesig, carvemask)
#define IS_FREESTANDING(pos) sigmatch((pos), freesig, freemask)

#define IS_MOB(pos) (mobmap[pos])
#define IS_SMARTMOB(tile, pos) ((flags_bin[tile] & 3) || mobmap[pos])
#define IS_MOB_AI(tile, pos)                                                   \
  ((flags_bin[tile] & 3) || (mobmap[pos] && !mob_active[mobmap[pos] - 1]))

#define URAND() ((u8)rand())

#define POS_TO_ADDR(pos) (0x9822 + (((pos)&0xf0) << 1) + ((pos)&0xf))
#define POS_TO_X(pos) (((pos & 0xf) << 3) + 24)
#define POS_TO_Y(pos) (((pos & 0xf0) >> 1) + 24)

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

#define TILE_EMPTY 1
#define TILE_WALL 2
#define TILE_WALL_FACE 3
#define TILE_WALL_FACE_PLANT 5
#define TILE_TELEPORTER 6
#define TILE_CARPET 7
#define TILE_VOID1 9
#define TILE_VOID2 10
#define TILE_END 0xd
#define TILE_START 0xe
#define TILE_GOAL 0x3e
#define TILE_TORCH_LEFT 0x3f
#define TILE_TORCH_RIGHT 0x41
#define TILE_HEART 0x42
#define TILE_DIRT1 0x43
#define TILE_DIRT2 0x44
#define TILE_DIRT3 0x48
#define TILE_PLANT1 0xb
#define TILE_PLANT2 0xc
#define TILE_PLANT3 0x47
#define TILE_FIXED_WALL 0x4a
#define TILE_SAW1 0x4e
#define TILE_WALL_FACE_CRACKED 0x60
#define TILE_STEPS 0x6a
#define TILE_ENTRANCE 0x6b

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
} PickupType;

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

const u8 dirx[] = {0xff, 1, 0, 0};       // L R U D
const u8 diry[] = {0, 0, 0xff, 1};       // L R U D
const u8 dirpos[] = {0xff, 1, 0xf0, 16}; // L R U D
const u8 dirvalid[] = {VALID_L,VALID_R,VALID_U,VALID_D};

// Movement is reversed since it is accessed backward
const u8 hopy4[] = {1, 1, 1, 1, 0xff, 0xff, 0xff, 0xff};

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
    0x59,                                     // vase1
    0x5f,                                     // vase2
    0x53,                                     // chest
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
    1,  // total
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
    52, // total
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
    0, // bomb
    1, // vase1
    1, // vase2
    1, // chest
};

const u8 float_diff_y[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
                           1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1};

const u8 float_dmg[] = {0, 0x1e, 0x2e, 0x3d};

void init(void);
void do_turn(void);
void pass_turn(void);
void begin_mob_anim(void);
void move_player(void);
void mobdir(u8 index, u8 dir);
void mobwalk(u8 index, u8 dir);
void mobbump(u8 index, u8 dir);
void mobhop(u8 index, u8 pos);
void hitmob(u8 index, u8 dmg);
void do_ai(void);
void do_ai_weeds(void);
u8 do_mob_ai(u8 index);
u8 ai_dobump(u8 index);
u8 ai_tcheck(u8 index);
u8 ai_getnextstep(u8 index);
u8 ai_getnextstep_rev(u8 index);
void sight(void);
void calcdist_ai(u8 from, u8 to);
void animate_mobs(void);
void end_mob_anim(void);
void set_tile_during_vbl(u8 pos, u8 tile);
void vbl_interrupt(void);

void addfloat(u8 pos, u8 tile);
void update_floats(void);

void fadeout(void);
void fadein(void);

void addmob(MobType type, u8 pos);
void delmob(u8 index);
void addpickup(PickupType type, u8 pos);

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

Map tmap;     // Tile map (everything unfogged)
Map dtmap;    // Display tile map (w/ fogged tiles)
Map roommap;  // Room/void map
Map distmap;  // Distance map
Map mobmap;   // Mob map
Map sigmap;   // Signature map (tracks neighbors) **only used during mapgen
Map tempmap;  // Temp map (used for carving)      **only used during mapgen
Map sightmap; // Sight from player
Map fogmap;   // Tiles player hasn't seen

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
u8 void_num_walls[MAX_VOIDS]; // Number of walls in a void region
u8 num_voids;

u8 start_room;
u8 floor;
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
u8 num_mobs;

u8 tile_code[TILE_CODE_SIZE];         // code for animating tiles
u8 mob_tile_code[MOB_TILE_CODE_SIZE]; // code for animating mobs
u8 tile_code_end;                     // next available index in tile_code
u16 last_tile_addr;                   // last tile address in HL for tile_code
u8 last_tile_val;                     // last tile value in A for tile_code
u8 tile_timer; // timer for animating tiles (every TILE_ANIM_SPEED frames)
u8 tile_inc;   // either 0 or TILE_ANIM_FRAME_DIFF, for two frames of animation
u8 mob_tile_code_end;   // next available index in mob_tile_code
u16 mob_last_tile_addr; // last tile address in HL for mob_tile_code
u8 mob_last_tile_val;   // last tile value in A for mob_tile_code

u8 float_time[MAX_FLOATS];
u8 num_floats;

Turn turn;
u8 noturn;

u8 key;

u8 doupdatemap;
u8 num_sprites;

u16 myclock;
void tim_interrupt(void) { ++myclock; }

void main(void) {
  init();
  mapgen();

  doupdatemap = 1;
  LCDC_REG = 0b10000011;  // display on, bg on, obj on
  fadein();

  while(1) {
    key = joypad();

    // XXX
    if (key & J_START) {
      if (++floor == 11) { floor = 0; }
      fadeout();
      IE_REG &= ~VBL_IFLAG;
      mapgen();
      // Reset the mob anims so they don't draw on this new map
      begin_mob_anim();
      IE_REG |= VBL_IFLAG;
      doupdatemap = 1;
      fadein();
    }

    begin_mob_anim();
    do_turn();
    animate_mobs();
    end_mob_anim();
    update_floats();
    wait_vbl_done();
  }
}

void init(void) {
  DISPLAY_OFF;

   // 0:LightGray 1:DarkGray 2:Black 3:White
  BGP_REG = OBP0_REG = OBP1_REG = fadepal[0];

  add_TIM(tim_interrupt);
  TMA_REG = 0;      // Divide clock by 256 => 16Hz
  TAC_REG = 0b100;  // 4096Hz, timer on.
  IE_REG |= TIM_IFLAG;

  enable_interrupts();

  floor = 0;
  initrand(0x4321);  // TODO: seed with DIV on button press

  set_sprite_data(0, SPRITES_TILES, sprites_bin);
  set_bkg_data(0, BG_TILES, bg_bin);
  set_bkg_data(0x80, SHARED_TILES, shared_bin);
  init_bkg(0);

  num_mobs = 0;
  addmob(MOB_TYPE_PLAYER, 0);

  doupdatemap = 0;

  turn = TURN_PLAYER;
  noturn = 0;

  num_floats = 0;
  num_sprites = 0;

  tile_inc = 0;
  tile_timer = TILE_ANIM_SPEED;
  tile_code[0] = mob_tile_code[0] = 0xc9; // ret
  add_VBL(vbl_interrupt);
}

void begin_mob_anim(void) {
  mob_last_tile_addr = 0;
  mob_last_tile_val = 0xff;
  mob_tile_code_end = 1;
  // Initialize to ret so if a VBL occurs while we're setting the animation it
  // won't run off the end
  mob_tile_code[0] = 0xc9;  // ret
}

void end_mob_anim(void) {
  mob_tile_code[mob_tile_code_end++] = 0xf1; // pop af
  mob_tile_code[mob_tile_code_end++] = 0xc9; // ret
  mob_tile_code[0] = 0xf5; // push af
}

void do_turn(void) {
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
    break;
  }
}

void move_player(void) {
  u8 dir, pos, newpos, tile;
  if (mob_move_timer[PLAYER_MOB] == 0) {
    if (key & (J_LEFT | J_RIGHT | J_UP | J_DOWN)) {
      if (key & J_LEFT) {
        dir = DIR_LEFT;
      } else if (key & J_RIGHT) {
        dir = DIR_RIGHT;
      } else if (key & J_UP) {
        dir = DIR_UP;
      } else {
        dir = DIR_DOWN;
      }

      pos = mob_pos[PLAYER_MOB];
      newpos = POS_DIR(pos, dir);
      mobdir(PLAYER_MOB, dir);
      if (IS_MOB(newpos)) {
        mobbump(PLAYER_MOB, dir);
        hitmob(mobmap[newpos] - 1, 1);
      } else if (validmap[pos] & dirvalid[dir]) {
        tile = tmap[newpos];
        if (IS_WALL_TILE(tile)) {
          mobbump(PLAYER_MOB, dir);
          noturn = 1;
        } else {
          mobwalk(PLAYER_MOB, dir);
        }
      } else {
        mobbump(PLAYER_MOB, dir);
        noturn = 1;
      }
      turn = TURN_PLAYER_WAIT;
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

  mob_move_timer[index] = WALK_TIME;
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

  mob_move_timer[index] = BUMP_TIME;
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

  mob_move_timer[index] = HOP_TIME;
  mob_anim_state[index] = MOB_ANIM_STATE_HOP4;
  mob_pos[index] = newpos;
  mobmap[pos] = 0;
  mobmap[newpos] = index + 1;
}

void hitmob(u8 index, u8 dmg) {
  u8 pos = mob_pos[index];
  if (mob_type_object[mob_type[index]]) {
    // TODO: handle chest, bomb
    // TODO: turn tile into dirt tile
    // TODO: randomly drop pickup or slime
    delmob(index);
    set_tile_during_vbl(pos, dtmap[pos] = tmap[pos] = dirt_tiles[rand() & 3]);
  } else {
    // TODO: flash mob for 10 frames
    addfloat(pos, float_dmg[dmg]);

    if (mob_hp[index] <= dmg) {
      // TODO: drop key, if any
      // TODO: restore to 5hp if reaper killed, and drop key
      delmob(index);
      set_tile_during_vbl(pos, dtmap[pos]);
    } else {
      mob_hp[index] -= dmg;
    }
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
      if (sightmap[pos]) {
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
      if (!sightmap[pos]) {
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
            mob_charge[index] = QUEEN_CHARGE_TIME;
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

  // Use sightmap as a quick check to determine whether this mob is close to
  // the player.
  if (sightmap[pos]) {
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
    return 1;
  }
  return 0;
}

u8 ai_tcheck(u8 index) {
  ++mob_ai_cool[index];
  if (sightmap[mob_pos[index]]) {
    mob_target_pos[index] = mob_pos[PLAYER_MOB];
    mob_ai_cool[index] = 0;
  }
  if (mob_target_pos[index] == mob_pos[index] ||
      mob_ai_cool[index] > AI_COOL_TIME) {
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
  u8 ppos, pos, adjpos, diff, head, oldtail, newtail, sig, valid, first;
  memset(sightmap, 0, sizeof(sightmap));

  // Put a "ret" at the beginning of the tile animation code in case it is
  // executed while we are modifying it.
  tile_code[0] = 0xc9;

  ppos = mob_pos[PLAYER_MOB];

  cands[head = 0] = 0;
  newtail = 1;
  first = 1;
  do {
    oldtail = newtail;
    do {
      diff = cands[head++];
      sightmap[pos = (u8)(ppos + diff)] = 1;

      // Unfog this tile
      if (fogmap[pos]) {
        set_tile_during_vbl(pos, dtmap[pos] = tmap[pos]);
        fogmap[pos] = 0;

        // Potentially start animating this tile
        if (IS_ANIMATED_TILE(tmap[pos])) {
          add_tile_anim(pos, tmap[pos]);
        }
      }

      if (first || !IS_OPAQUE_TILE(tmap[pos])) {
        first = 0;
        // Unfog neighbors
        valid = validmap[pos];
        if ((valid & VALID_U) && fogmap[adjpos = POS_U(pos)] &&
            IS_WALL_TILE(tmap[adjpos])) {
          set_tile_during_vbl(adjpos, dtmap[adjpos] = tmap[adjpos]);
          fogmap[adjpos] = 0;
        }
        if ((valid & VALID_L) && fogmap[adjpos = POS_L(pos)] &&
            IS_WALL_TILE(tmap[adjpos])) {
          set_tile_during_vbl(adjpos, dtmap[adjpos] = tmap[adjpos]);
          fogmap[adjpos] = 0;
        }
        if ((valid & VALID_R) && fogmap[adjpos = POS_R(pos)] &&
            IS_WALL_TILE(tmap[adjpos])) {
          set_tile_during_vbl(adjpos, dtmap[adjpos] = tmap[adjpos]);
          fogmap[adjpos] = 0;
        }
        if ((valid & VALID_D) && fogmap[adjpos = POS_D(pos)] &&
            IS_WALL_TILE(tmap[adjpos])) {
          set_tile_during_vbl(adjpos, dtmap[adjpos] = tmap[adjpos]);
          fogmap[adjpos] = 0;
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
  } while (oldtail != newtail);

  // Put the epilog back on, and replace the "ret" with "push af"
  end_tile_anim();
  tile_code[0] = 0xf5;
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

void animate_mobs(void) {
  u8 i, dotile, frame, animdone, last_num_sprites;

  animdone = 1;
  last_num_sprites = num_sprites;
  num_sprites = MAX_FLOATS;  // Start mob sprites after floats

  // Loop through all mobs, update animations
  for (i = 0; i < num_mobs; ++i) {
    if (fogmap[mob_pos[i]]) { continue; }

    dotile = 0;
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

    if (dotile || mob_move_timer[i]) {
      frame = mob_type_anim_frames[mob_anim_frame[i]];
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

        set_sprite_tile(num_sprites, frame);
        move_sprite(num_sprites, mob_x[i], mob_y[i]);
        ++num_sprites;

        if (mob_move_timer[i] == 0) {
          switch (mob_anim_state[i]) {
            case MOB_ANIM_STATE_BUMP1:
              mob_anim_state[i] = MOB_ANIM_STATE_BUMP2;
              mob_move_timer[i] = BUMP_TIME;
              mob_dx[i] = -mob_dx[i];
              mob_dy[i] = -mob_dy[i];
              break;

            case MOB_ANIM_STATE_WALK:
              if (i == PLAYER_MOB) { sight(); }
              mob_anim_speed[i] = mob_type_anim_speed[mob_type[i]];
              goto done;

            done:
            case MOB_ANIM_STATE_BUMP2:
            case MOB_ANIM_STATE_HOP4:
              mob_anim_state[i] = MOB_ANIM_STATE_NONE;
              mob_dx[i] = mob_dy[i] = 0;
              set_tile_during_vbl(mob_pos[i], frame);
              break;
          }
        }
      } else {
        set_tile_during_vbl(mob_pos[i], frame);
      }
    }
  }

  // Hide the rest of the sprites
  for (i = num_sprites; i < last_num_sprites; ++i) {
    hide_sprite(i);
  }

  if (animdone) {
    pass_turn();
  }
}

void set_tile_during_vbl(u8 pos, u8 tile) {
  u16 addr = POS_TO_ADDR(pos);
  u8 *ptr = mob_tile_code + mob_tile_code_end;
  if (mob_tile_code_end == 1 || (addr >> 8) != (mob_last_tile_addr >> 8)) {
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
  mob_tile_code_end = ptr - mob_tile_code;
}

void vbl_interrupt(void) {
  if (doupdatemap) {
    set_bkg_tiles(2, 1, MAP_WIDTH, MAP_HEIGHT, dtmap);
    doupdatemap = 0;
  }
  if (--tile_timer == 0) {
    tile_timer = TILE_ANIM_SPEED;
    tile_inc ^= TILE_ANIM_FRAME_DIFF;
    ((vfp)tile_code)();
  }

  ((vfp)mob_tile_code)();
}

void addfloat(u8 pos, u8 tile) {
  shadow_OAM[num_floats].x = POS_TO_X(pos);
  shadow_OAM[num_floats].y = POS_TO_Y(pos);
  shadow_OAM[num_floats].tile = tile;
  float_time[num_floats] = FLOAT_TIME;
  ++num_floats;
}

void update_floats(void) {
  u8 i;
  for (i = 0; i < num_floats;) {
    --float_time[i];
    if (float_time[i] == 0) {
      --num_floats;
      if (num_floats) {
        shadow_OAM[i].x = shadow_OAM[num_floats].x;
        shadow_OAM[i].y = shadow_OAM[num_floats].y;
        shadow_OAM[i].tile = shadow_OAM[num_floats].tile;
        float_time[i] = float_time[num_floats];
        hide_sprite(num_floats);
      } else {
        hide_sprite(i);
      }
      continue;
    } else {
      shadow_OAM[i].y -= float_diff_y[float_time[i]];
    }
    ++i;
  }
}

void fadeout(void) {
  u8 i, j;
  i = 3;
  do {
    for (j = 0; j < FADE_TIME; ++j) { wait_vbl_done(); }
    BGP_REG = OBP0_REG = OBP1_REG = fadepal[i];
  } while(i--);
}

void fadein(void) {
  u8 i, j;
  i = 0;
  do {
    for (j = 0; j < FADE_TIME; ++j) { wait_vbl_done(); }
    BGP_REG = OBP0_REG = OBP1_REG = fadepal[i];
  } while(++i != 4);
}

u8 dummy[13];
#define TIMESTART dummy[0] = (u8)(myclock)
#define TIMEDIFF(x)                                                            \
  dummy[x] = (u8)(myclock)-dummy[0];                                           \
  TIMESTART

void addmob(MobType type, u8 pos) {
  mob_type[num_mobs] = type;
  mob_anim_frame[num_mobs] = mob_type_anim_start[type];
  mob_anim_timer[num_mobs] = 1;
  mob_anim_speed[num_mobs] = mob_type_anim_speed[type];
  mob_pos[num_mobs] = pos;
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

void addpickup(PickupType pick, u8 pos) {
  // TODO
  (void)pick;
  (void)pos;
}

void initdtmap(void) {
  u8 pos = 0;
  do {
    dtmap[pos] = fogmap[pos] ? 0 : tmap[pos];
  } while(++pos);
}

void mapgen(void) {
  u8 fog;
  num_mobs = 1;
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
    TIMESTART;
    roomgen();
    TIMEDIFF(1);
    mazeworms();
    TIMEDIFF(2);
    carvedoors();
    TIMEDIFF(3);
    carvecuts();
    TIMEDIFF(4);
    startend();
    TIMEDIFF(5);
    fillends();
    TIMEDIFF(6);
    prettywalls();
    TIMEDIFF(7);
    voids();
    TIMEDIFF(8);
    chests();
    TIMEDIFF(9);
    decoration();
    TIMEDIFF(10);
    spawnmobs();
    TIMEDIFF(11);

    dummy[12] = dummy[1] + dummy[2] + dummy[3] + dummy[4] + dummy[5] +
                dummy[6] + dummy[7] + dummy[8] + dummy[9] + dummy[10] + dummy[11];
  }
  end_tile_anim();
  memset(fogmap, fog, sizeof(fogmap));
  sight();
  initdtmap();
}

void mapgeninit(void) {
  memset(roommap, 0, sizeof(roommap));
  memset(mobmap, 0, sizeof(mobmap));
  memset(sigmap, 255, sizeof(sigmap));
  memset(ds_sets, 0, sizeof(ds_sets));

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
    copymap(2 + randint(14));
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
  u8 pos, valid;

  memcpy(tmap, map_bin + (index << 8), 256);

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

      case TILE_GOAL:
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
        ((URAND() & 1) && step > 2)) {
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
    tmap[gatepos] = TILE_GOAL;
    if (floor < 9) {
      addpickup(PICKUP_TYPE_HEART, heartpos);
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
        tile += 14;
      }
      if (TILE_HAS_CRACKED_VARIANT(tile) && !(URAND() & 7)) {
        tile += 0x35;
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
  u8 pos, head, oldtail, newtail, valid, newpos;
  pos = 0;
  num_voids = 0;

  memset(void_num_tiles, 0, sizeof(void_num_tiles));

  do {
    if (!tmap[pos]) {
      // flood fill this region
      cands[head = 0] = pos;
      newtail = 1;
      do {
        oldtail = newtail;
        do {
          pos = cands[head++];
          if (!tmap[pos]) {
            roommap[pos] = num_rooms + num_voids + 1;
            ++void_num_tiles[num_voids];
            tmap[pos] = void_tiles[randint(sizeof(void_tiles))];
            valid = validmap[pos];

            // TODO: handle void exits
            if ((valid & VALID_U) && !tmap[newpos = POS_U(pos)]) {
              cands[newtail++] = newpos;
            }
            if ((valid & VALID_L) && !tmap[newpos = POS_L(pos)]) {
              cands[newtail++] = newpos;
            }
            if ((valid & VALID_R) && !tmap[newpos = POS_R(pos)]) {
              cands[newtail++] = newpos;
            }
            if ((valid & VALID_D) && !tmap[newpos = POS_D(pos)]) {
              cands[newtail++] = newpos;
            }
          }
        } while(head != oldtail);
      } while (oldtail != newtail);
      ++num_voids;
    }
  } while(++pos);
}

void chests(void) {
  u8 has_teleporter, room, chest_pos, i, pos, cand0, cand1;

  // Teleporter in level if level >= 5 and rnd(5)<1 (20%)
  has_teleporter = floor >= 5 && URAND() < 51;

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
    addmob(MOB_TYPE_CHEST, cands[i]);
    // TODO: randomly make some chests heart containers
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
            if (tile == TILE_EMPTY && (URAND() < 85) && (h & 1) &&
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
                !(URAND() & 3) && no_mob_neighbors(pos) &&
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
            URAND() < 26) {
          tmap[pos] = TILE_SAW1;
          sigwall(pos);
          // TODO: what to do about removing animations on broken saws?
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
  tile_code_end = 6;
  last_tile_addr = 0;
  last_tile_val = 0;
}

void end_tile_anim(void) {
  u8 *ptr = tile_code + tile_code_end;
  *ptr++ = 0xc1; // pop bc
  *ptr++ = 0xf1; // pop af
  *ptr = 0xc9;   // ret
}

void add_tile_anim(u8 pos, u8 tile) {
  u8* ptr = tile_code + tile_code_end;
  u16 addr = POS_TO_ADDR(pos);
  if (tile_code_end == 6 || (addr >> 8) != (last_tile_addr >> 8)) {
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
  tile_code_end = ptr - tile_code;
}

void spawnmobs(void) {
  u8 pos, room, mobs;

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

  // TODO: give a mob a key, if this is a gate floor
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

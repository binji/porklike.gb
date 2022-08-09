#ifndef COMMON_H_
#define COMMON_H_

#include <stdint.h>

#ifdef __SDCC
#define PRESERVES_REGS(...) __preserves_regs(__VA_ARGS__)
#else
#define PRESERVES_REGS(...)
#endif

//
// TODO move a bunch of this stuff to their own files
//

#define MAP_WIDTH 16
#define MAP_HEIGHT 16

#define POS_UL(pos)       ((u8)(pos + 239))
#define POS_U(pos)        ((u8)(pos + 240))
#define POS_UR(pos)       ((u8)(pos + 241))
#define POS_L(pos)        ((u8)(pos + 255))
#define POS_R(pos)        ((u8)(pos + 1))
#define POS_DL(pos)       ((u8)(pos + 15))
#define POS_D(pos)        ((u8)(pos + 16))
#define POS_DR(pos)       ((u8)(pos + 17))
#define POS_DIR(pos, dir) ((u8)(pos + dirpos[dir]))

#define POS_TO_ADDR(pos) (0x9800 + (((pos)&0xf0) << 1) + ((pos)&0xf))
#define POS_TO_X(pos) (((pos & 0xf) << 3) + 24)
#define POS_TO_Y(pos) (((pos & 0xf0) >> 1) + 16)

// Hide sprites under this y position, so they aren't displayed on top of the
// inventory
#define INV_TOP_Y() (WY_REG + 9)

#define IS_WALL_FLAG(flag)             ((flag) & 0b00000001)
#define IS_SPECIAL_FLAG(flag)          ((flag) & 0b00000010)
#define IS_WALL_OR_SPECIAL_FLAG(flag)  ((flag) & 0b00000011)
#define IS_OPAQUE_FLAG(flag)           ((flag) & 0b00000100)
#define IS_CRACKED_WALL_FLAG(flag)     ((flag) & 0b00001000)
#define IS_ANIMATED_FLAG(flag)         ((flag) & 0b00010000)
#define IS_WALL_FACE_FLAG(flag)        ((flag) & 0b00100000)
#define FLAG_HAS_CRACKED_VARIANT(flag) ((flag) & 0b01000000)
#define IS_BREAKABLE_WALL_FLAG(flag)   ((flag) & 0b10000000)

#define IS_WALL_TILE(tile)             IS_WALL_FLAG(flags_bin[tile])
#define IS_SPECIAL_TILE(tile)          IS_SPECIAL_FLAG(flags_bin[tile])
#define IS_WALL_OR_SPECIAL_TILE(tile)  IS_WALL_OR_SPECIAL_FLAG(flags_bin[tile])
#define IS_OPAQUE_TILE(tile)           IS_OPAQUE_FLAG(flags_bin[tile])
#define IS_CRACKED_WALL_TILE(tile)     IS_CRACKED_WALL_FLAG(flags_bin[tile])
#define IS_ANIMATED_TILE(tile)         IS_ANIMATED_FLAG(flags_bin[tile])
#define IS_WALL_FACE_TILE(tile)        IS_WALL_FACE_FLAG(flags_bin[tile])
#define TILE_HAS_CRACKED_VARIANT(tile) FLAG_HAS_CRACKED_VARIANT(flags_bin[tile])
#define IS_BREAKABLE_WALL_TILE(tile)   IS_BREAKABLE_WALL_FLAG(flags_bin[tile])
#define IS_SMARTMOB_TILE(tile, pos) (IS_WALL_OR_SPECIAL_TILE(tile) || mobmap[pos])

// Optimizations for fetching tile flags;
// NOTE: cannot be used during level generation! The flagmap is not initialized
// and overlaps with the sigmap
#define IS_WALL_POS(pos)             IS_WALL_FLAG(flagmap[pos])
#define IS_SPECIAL_POS(pos)          IS_SPECIAL_FLAG(flagmap[pos])
#define IS_OPAQUE_POS(pos)           IS_OPAQUE_FLAG(flagmap[pos])
#define IS_CRACKED_WALL_POS(pos)     IS_CRACKED_WALL_FLAG(flagmap[pos])
#define IS_ANIMATED_POS(pos)         IS_ANIMATED_FLAG(flagmap[pos])
#define IS_WALL_FACE_POS(pos)        IS_WALL_FACE_FLAG(flagmap[pos])
#define POS_HAS_CRACKED_VARIANT(pos) FLAG_HAS_CRACKED_VARIANT(flagmap[pos])
#define IS_BREAKABLE_WALL_POS(pos)   IS_BREAKABLE_WALL_FLAG(flagmap[pos])
#define IS_SMARTMOB_POS(pos) (IS_WALL_OR_SPECIAL_FLAG(flagmap[pos]) || mobmap[pos])

typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef void (*vfp)(void);

typedef u8 Map[MAP_WIDTH * MAP_HEIGHT];

typedef enum Dir {
  DIR_LEFT,
  DIR_RIGHT,
  DIR_UP,
  DIR_DOWN,
} Dir;

typedef enum Turn {
  TURN_PLAYER,
  TURN_PLAYER_MOVED,
  TURN_AI,
  TURN_WEEDS,
} Turn;

extern const u8 permutation_4[];
extern const u8 dirx[];
extern const u8 diry[];
extern const u8 dirx2[];
extern const u8 diry2[];
extern const u8 dirx3[];
extern const u8 diry3[];
extern const u8 dirx4[];
extern const u8 diry4[];
extern const u8 dirpos[];
extern const u8 hopy4[];
extern const u8 hopy12[];
extern const u8 pickbounce[];

extern const u8 flags_bin[];

extern const u8 dirvalid[];
extern const u8 validmap[];

extern Map distmap;
extern Map fogmap;
extern Map flagmap;
extern Map mobsightmap;

extern u8 *next_sprite, *last_next_sprite;

extern Turn turn;
extern u8 dopassturn, doai;
extern u8 noturn;

extern u8 joy_action; // The most recently pressed action button

void dirty_tile(u8 pos);
u8 ai_dobump(u8 index);
void trigger_step(u8 index);
u8 dropspot(u8 pos);
void addfloat(u8 pos, u8 tile);
void hitmob(u8 index, u8 dmg);
void blind(void);


#endif // COMMON_H_

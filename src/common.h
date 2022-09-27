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

#define IS_MOB(pos) (mobmap[pos])
#define IS_WALL_OR_MOB(pos) (IS_WALL_POS(pos) || mobmap[pos])

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
#define TILE_KIELBASA 0x6e
#define TILE_PLANT1 0xc
#define TILE_PLANT2 0xd
#define TILE_PLANT3 0x5c
#define TILE_FIXED_WALL 0x47
#define TILE_SAW 0x42
#define TILE_SAW_MASK 0xef
#define TILE_SAW_BROKEN 0x53
#define TILE_STEPS 0x60
#define TILE_ENTRANCE 0x61

// Shared tiles
#define FLOAT_FRAMES 70

#define MAX_EQUIPS 4

#define NUM_BOOM_SPRS 16

#define MAX_WURSTCHAIN 7

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

extern const u8 permutation_4[];
extern const u8 n_over_3[];
extern const u16 three_over_n[];

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
extern const Dir invdir[];

extern const u8 flags_bin[];

extern const u8 dirvalid[];
extern const u8 validmap[];

extern const u8 dirt_tiles[];

extern Map tmap;
extern Map distmap;
extern Map fogmap;
extern Map flagmap;
extern Map mobsightmap;
extern Map roommap;
extern Map dtmap;
extern Map sawmap;
extern Map dirtymap;

extern u8 num_rooms;

extern u8 void_exit[];

// Info used for debugging generated levels
extern u8 dogate;
extern u16 floor_seed;

extern u8 startpos;
extern u8 floor;

void vram_copy(u16 dst, void* src, u8 len);

u8 is_new_pos_valid(u8 pos, u8 diff) PRESERVES_REGS(b, c);
extern u8 pos_result;

void clear_map(u8* map);

#endif // COMMON_H_

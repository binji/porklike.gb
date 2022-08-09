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

extern Map fogmap;

extern u8 *next_sprite, *last_next_sprite;

void dirty_tile(u8 pos);
u8 ai_dobump(u8 index);
void trigger_step(u8 index);


#endif // COMMON_H_

#ifndef GAMEPLAY_H_
#define GAMEPLAY_H_

#include "common.h"
#include "counter.h"
#include "spr.h"

typedef enum Turn {
  TURN_PLAYER,
  TURN_PLAYER_MOVED,
  TURN_AI,
  TURN_WEEDS,
} Turn;

void gameplay_init(void);
void gameplay_update(void);

void trigger_step(u8 mob);
void sight(void);
void blind(void);

void hitmob(u8 index, u8 dmg);
void hitpos(u8 pos, u8 dmg, u8 stun);
void dirty_tile(u8 pos);
void trigger_spr(SprType, u8 trigger_val);
u8 dropspot(u8 pos);

void sram_init(void);
void sram_update_wurstchain(u8 value);

extern Turn turn;

extern u8 recover; // how long until recovering from blind
extern u16 steps;
extern u8 num_keys;
extern u8 doupdatemap, doloadfloor;

extern Counter st_floor;
extern Counter st_steps;
extern Counter st_kills;
extern Counter st_recover;

extern u8 wurstchain;

#endif // GAMEPLAY_H_

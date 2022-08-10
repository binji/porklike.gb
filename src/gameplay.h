#ifndef GAMEPLAY_H_
#define GAMEPLAY_H_

#include "common.h"
#include "counter.h"

typedef enum Turn {
  TURN_PLAYER,
  TURN_PLAYER_MOVED,
  TURN_AI,
  TURN_WEEDS,
} Turn;

void do_turn(void);
void pass_turn(void);

void move_player(void);
void use_pickup(void);
u8 shoot(u8 pos, u8 hit, u8 tile, u8 prop);
u8 shoot_dist(u8 pos, u8 hit);
u8 rope(u8 from, u8 to);

void trigger_step(u8 mob);
void sight(void);
void blind(void);

void hitmob(u8 index, u8 dmg);
void hitpos(u8 pos, u8 dmg, u8 stun);
void update_wall_face(u8 pos);
void dirty_tile(u8 pos);
void update_tile(u8 pos, u8 tile);
void unfog_tile(u8 pos);

extern Turn turn;
extern u8 dopassturn, doai;

extern u8 recover; // how long until recovering from blind
extern u16 steps;
extern u8 num_keys;

extern Counter st_floor;
extern Counter st_steps;
extern Counter st_kills;
extern Counter st_recover;

#endif // GAMEPLAY_H_

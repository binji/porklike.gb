#ifndef PICKUP_H_
#define PICKUP_H_

#include "common.h"

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

void addpick(PickupType type, u8 pos);
void delpick(u8 index);
u8 animate_pickups(void);

void droppick(PickupType type, u8 pos);
void droppick_rnd(u8 pos);

void pickhop(u8 index, u8 pos);

extern Map pickmap;
extern u8 num_picks;

extern PickupType pick_type[];
extern u8 pick_tile[]; // Actual tile index
extern u8 pick_pos[];
extern u8 pick_anim_frame[]; // Index into pick_type_anim_frames
extern u8 pick_anim_timer[]; // 0..pick_type_anim_speed[type]
extern u8 pick_x[];          // x location of sprite
extern u8 pick_y[];          // y location of sprite
extern u8 pick_dx[];         // dx moving sprite
extern u8 pick_dy[];         // dy moving sprite
extern u8 pick_move_timer[]; // timer for sprite animation

// XXX move to UI/inventory
extern PickupType inv_selected_pick;

#endif // PICKUP_H_

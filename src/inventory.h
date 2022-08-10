#ifndef INVENTORY_H_
#define INVENTORY_H_

#include "common.h"
#include "pickup.h"

#define NUM_INV_ROWS 4

// Hide sprites under this y position, so they aren't displayed on top of the
// inventory
#define INV_TOP_Y() (WY_REG + 9)

void inv_init(void);
void inv_update(void);
void inv_animate(void);

void inv_display_blind();
void inv_display_floor();
void inv_decrement_recover(void);
void inv_update_hp(void);
void inv_update_keys(void);

void inv_open(void);
void inv_close(void);

u8 inv_acquire_pickup(PickupType ptype);
void inv_use_pickup(void);

extern u8 inv_anim_up;
extern u8 inv_anim_timer;

#endif // INVENTORY_H_

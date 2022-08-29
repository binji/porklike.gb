#ifndef SPR_H_
#define SPR_H_

#include "common.h"

typedef enum SprType {
  SPR_TYPE_NONE,
  SPR_TYPE_BOOM,
  SPR_TYPE_BOLT,
  SPR_TYPE_PUSH,
  SPR_TYPE_GRAPPLE,
  SPR_TYPE_HOOK,
  SPR_TYPE_SPIN,
} SprType;

u8 spr_add(u8 speed, u16 x, u16 y, u16 dx, u16 dy, u8 drag, u8 timer, u8 prop);
void spr_update(void);
void spr_hide(void);

extern SprType spr_type[];
extern u8 spr_anim_frame[];  // Actual sprite tile (not index)
extern u8 spr_trigger_val[]; // Which value to use for trigger action
extern u8 num_sprs;

#endif // SPR_H_

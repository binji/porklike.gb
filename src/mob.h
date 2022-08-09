#ifndef MOB_H_
#define MOB_H_

#include "common.h"

#define PLAYER_MOB 0

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

typedef enum MobAI {
  MOB_AI_NONE,
  MOB_AI_WAIT,
  MOB_AI_WEED,
  MOB_AI_REAPER,
  MOB_AI_ATTACK,
  MOB_AI_QUEEN,
  MOB_AI_KONG,
} MobAI;

typedef enum MobAnimState {
  MOB_ANIM_STATE_NONE = 0,
  MOB_ANIM_STATE_WALK = 1,
  MOB_ANIM_STATE_BUMP1 = 2,
  MOB_ANIM_STATE_BUMP2 = 3,
  MOB_ANIM_STATE_HOP4 = 4,
  MOB_ANIM_STATE_HOP4_MASK = 7,
  // Use 8 here so we can use & to check for HOP4 or POUNCE
  MOB_ANIM_STATE_POUNCE = MOB_ANIM_STATE_HOP4 | 8,
} MobAnimState;

void addmob(MobType type, u8 pos);
void delmob(u8 index);
u8 animate_mobs(void);

void adddeadmob(u8 index);
void animate_deadmobs(void);

void mobdir(u8 index, u8 dir);
void mobwalk(u8 index, u8 dir);
void mobbump(u8 index, u8 dir);
void mobhop(u8 index, u8 pos);
void mobhopnew(u8 index, u8 pos);

extern Map mobmap;

extern const u8 mob_type_hp[];
extern const u8 mob_type_anim_speed[];
extern const u8 mob_type_anim_frames[];
extern const u8 mob_type_anim_speed[];
extern const u8 mob_type_anim_start[];
extern const MobAI mob_type_ai_wait[];
extern const MobAI mob_type_ai_active[];
extern const u8 mob_type_object[];

extern u8 mob_pos[];
extern u8 num_mobs;
extern u8 num_dead_mobs;
extern u8 key_mob;

extern MobType mob_type[];
extern u8 mob_tile[];       // Actual tile index
extern u8 mob_anim_frame[]; // Index into mob_type_anim_frames
extern u8 mob_anim_timer[]; // 0..mob_anim_speed[type], how long between frames
extern u8 mob_anim_speed[]; // current anim speed (changes when moving)
extern u8 mob_pos[];
extern u8 mob_x[];                    // x location of sprite
extern u8 mob_y[];                    // y location of sprite
extern u8 mob_dx[];                   // dx moving sprite
extern u8 mob_dy[];                   // dy moving sprite
extern u8 mob_move_timer[];           // timer for sprite animation
extern MobAnimState mob_anim_state[]; // sprite animation state
extern u8 mob_flip[];                 // 0=facing right 1=facing left
extern MobAI mob_task[];              // AI routine
extern u8 mob_target_pos[];           // where the mob last saw the player
extern u8 mob_ai_cool[]; // cooldown time while mob is searching for player
extern u8 mob_active[];  // 0=inactive 1=active
extern u8 mob_charge[];  // only used by queen
extern u8 mob_hp[];
extern u8 mob_flash[];
extern u8 mob_stun[];
extern u8 mob_trigger[]; // Trigger a step action after this anim finishes?
extern u8 mob_vis[];     // Whether mob is visible on fogged tiles (for ghosts)

#endif // MOB_H_

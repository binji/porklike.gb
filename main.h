#ifndef MAIN_H_
#define MAIN_H_

#include <gb/gb.h>

#define MAP_WIDTH 16
#define MAP_HEIGHT 16

#define NUM_INV_ROWS 4
#define NUM_BOOM_SPRS 16

#define MAX_ROOMS 4

#define PLAYER_MOB 0

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

#define XRND_10_PERCENT() (xrnd() < 26)
#define XRND_12_5_PERCENT() ((xrnd() & 7) == 0)
#define XRND_20_PERCENT() (xrnd() < 51)
#define XRND_25_PERCENT() ((xrnd() & 3) == 0)
#define XRND_33_PERCENT() (xrnd() < 85)
#define XRND_50_PERCENT() (xrnd() & 1)

#define IS_WALL_TILE(tile)             (flags_bin[tile] & 0b00000001)
#define IS_SPECIAL_TILE(tile)          (flags_bin[tile] & 0b00000010)
#define IS_WALL_OR_SPECIAL_TILE(tile)  (flags_bin[tile] & 0b00000011)
#define IS_OPAQUE_TILE(tile)           (flags_bin[tile] & 0b00000100)
#define IS_CRACKED_WALL_TILE(tile)     (flags_bin[tile] & 0b00001000)
#define IS_ANIMATED_TILE(tile)         (flags_bin[tile] & 0b00010000)
#define IS_WALL_FACE_TILE(tile)        (flags_bin[tile] & 0b00100000)
#define TILE_HAS_CRACKED_VARIANT(tile) (flags_bin[tile] & 0b01000000)
#define IS_BREAKABLE_WALL(tile)        (flags_bin[tile] & 0b10000000)

#define IS_SMARTMOB(tile, pos) (IS_WALL_OR_SPECIAL_TILE(tile) || mobmap[pos])

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
#define TILE_PLANT1 0xc
#define TILE_PLANT2 0xd
#define TILE_PLANT3 0x5c
#define TILE_FIXED_WALL 0x47
#define TILE_SAW 0x42
#define TILE_SAW_BROKEN 0x53
#define TILE_STEPS 0x60
#define TILE_ENTRANCE 0x61

typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef void (*vfp)(void);

typedef u8 Map[MAP_WIDTH * MAP_HEIGHT];

typedef struct Counter {
  u8 start;   // First digit index in data[]
  u8 data[5]; // Digits as tile indexes, left padded
} Counter;

typedef enum Dir {
  DIR_LEFT,
  DIR_RIGHT,
  DIR_UP,
  DIR_DOWN,
} Dir;

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

extern const u8 flags_bin[];

extern const u8 fadepal[];
extern const u8 obj_pal1[];
extern const Dir invdir[];
extern const u8 dirx[];
extern const u8 diry[];
extern const u8 dirx2[];
extern const u8 diry2[];
extern const u8 dirx3[];
extern const u8 diry3[];
extern const u8 dirx4[];
extern const u8 diry4[];
extern const u8 n_over_3[];
extern const u16 three_over_n[];

extern const u8 hopy4[];
extern const u8 hopy12[];
extern const u8 pickbounce[];
extern const u8 sightsig[];

extern const u8 dirpos[];
extern const u8 dirvalid[];
extern const u8 validmap[];

extern const u8 mob_type_hp[];
extern const u8 mob_type_anim_speed[];
extern const u8 mob_type_anim_frames[];
extern const u8 mob_type_anim_speed[];
extern const u8 mob_type_anim_start[];
extern const MobAI mob_type_ai_wait[];
extern const MobAI mob_type_ai_active[];
extern const u8 mob_type_object[];

extern const u8 dirt_tiles[];

extern const u8 pick_type_anim_frames[];
extern const u8 pick_type_anim_start[];
extern const u8 pick_type_sprite_tile[];
extern const u8 pick_type_name_tile[];
extern const u8 pick_type_name_start[];
extern const u8 pick_type_name_len[];
extern const u8 pick_type_desc_tile[];
extern const u16 pick_type_desc_start[][NUM_INV_ROWS];
extern const u8 pick_type_desc_len[][NUM_INV_ROWS];

extern const u8 float_diff_y[];
extern const u8 float_dmg[];
extern const u8 float_pick_type_tiles[];
extern const u8 float_pick_type_start[];
extern const u8 float_pick_type_x_offset[];

extern const u8 drop_diff[];

extern const u8 inventory_map[];
extern const u8 inventory_up_y[];
extern const u8 inventory_down_y[];

extern const u8 msg_tiles[];

extern const u8 boom_spr_speed[];
extern const u8 boom_spr_anim_speed[];
extern const u16 boom_spr_x[];
extern const u16 boom_spr_y[];
extern const u16 boom_spr_dx[];
extern const u16 boom_spr_dy[];

extern const u8 gameover_map[];

void soundinit(void);

void titlescreen(void);
void clear_wram(void) __preserves_regs(b, c);
u16 drag(u16 x);
u8 xrnd(void) __preserves_regs(b, c);
void xrnd_init(u16) __preserves_regs(b, c);
void counter_zero(Counter* c) __preserves_regs(b, c);
void counter_inc(Counter* c) __preserves_regs(b, c);
void counter_out(Counter* c, u16 vram);

void fadeout(void);
void fadein(void);

void mapgen(void);
void sight(void);
void addmob(MobType type, u8 pos);
void addpick(PickupType type, u8 pos);
u8 randint(u8 mx);
void begin_tile_anim(void);
void end_tile_anim(void);
void add_tile_anim(u8 pos, u8 tile);

extern u8 joy, lastjoy, newjoy;

extern Map tmap;
extern Map dtmap;
extern Map roommap;
extern Map distmap;
extern Map mobmap;
extern Map pickmap;
extern Map sigmap;
extern Map tempmap;
extern Map fogmap;
extern Map sawmap;

extern u8 cands[];
extern u8 num_cands;

extern u8 room_pos[];
extern u8 room_w[];
extern u8 room_h[];
extern u8 room_avoid[];
extern u8 num_rooms;

extern u8 mob_pos[];
extern u8 num_mobs;
extern u8 key_mob;

extern u8 void_exit[];

extern u8 num_dead_mobs;
extern u8 start_room;
extern u8 num_picks;
extern u8 floor;
extern u8 startpos;
extern u16 steps;

extern MobType mob_type[];
extern u8 mob_anim_timer[];
extern u8 mob_anim_speed[];
extern u8 mob_move_timer[];
extern u8 mob_flip[];

#endif // MAIN_H_

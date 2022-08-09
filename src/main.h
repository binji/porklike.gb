#ifndef MAIN_H_
#define MAIN_H_

#ifdef __SDCC
#include <gb/gb.h>
#endif

#include "common.h"

#define NUM_INV_ROWS 4
#define NUM_BOOM_SPRS 16

#define MAX_ROOMS 4

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

extern const u8 fadepal[];
extern const u8 obj_pal1[];
extern const Dir invdir[];
extern const u8 n_over_3[];
extern const u16 three_over_n[];

extern const u8 sightsig[];

extern const u8 dirt_tiles[];

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

extern const u8 blind_map[];

extern const u8 msg_tiles[];

extern const u8 boom_spr_speed[];
extern const u8 boom_spr_anim_speed[];
extern const u16 boom_spr_x[];
extern const u16 boom_spr_y[];
extern const u16 boom_spr_dx[];
extern const u16 boom_spr_dy[];

extern const u8 dead_map[];
extern const u8 win_map[];
extern const u8 stats_map[];

void titlescreen(void);
void clear_wram(void) PRESERVES_REGS(b, c);
u16 drag(u16 x);

void vram_copy(u16 dst, void* src, u8 len);

void hide_sprites(void);
void fadeout(void);
void fadein(void);

void mapgen(void);
void sight(void);

extern u8 joy, lastjoy, newjoy;

extern Map tmap;
extern Map dtmap;
extern Map dirtymap;
extern Map roommap;
extern Map sigmap;
extern Map tempmap;
extern Map sawmap;

extern u8 cands[];
extern u8 num_cands;

extern u8 room_pos[];
extern u8 room_w[];
extern u8 room_h[];
extern u8 room_avoid[];
extern u8 num_rooms;

extern u8 dirty[];
extern u8 *dirty_ptr;

extern u8 anim_tiles[];
extern u8 *anim_tile_ptr;

extern u8 void_exit[];

extern u8 start_room;
extern u8 floor;
extern u8 startpos;
extern u16 steps;

// Info used for debugging generated levels
extern u16 floor_seed;
extern u8 dogate;

#endif // MAIN_H_

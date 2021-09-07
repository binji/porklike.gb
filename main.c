#include <gb/gb.h>
#include <rand.h>
#include <string.h>

#include "tilebg.c"
#include "tileshared.c"
#include "tilesprites.c"
#include "map.c"
#include "flags.c"

typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef void (*vfp)(void);

#define MAX_DS_SET 64  /* XXX can be smaller */
#define MAX_ROOMS 4
#define MAX_VOIDS 10 /* XXX Figure out what this should be */
#define MAX_CANDS 256
#define MAX_MOBS 32  /* XXX Figure out what this should be */

#define PLAYER_MOB 0

#define MAP_WIDTH 16
#define MAP_HEIGHT 16

#define TILE_ANIM_SPEED 8
#define TILE_ANIM_FRAME_DIFF 16

#define IS_WALL_TILE(tile) (flags_bin[tile] & 1)
#define TILE_HAS_CRACKED_VARIANT(tile) (flags_bin[tile] & 0b01000000)
#define IS_CRACKED_WALL_TILE(tile) (flags_bin[tile] & 0b00001000)

#define IS_DOOR(pos) sigmatch((pos), doorsig, doormask)
#define CAN_CARVE(pos) sigmatch((pos), carvesig, carvemask)
#define IS_FREESTANDING(pos) sigmatch((pos), freesig, freemask)

#define IS_SMARTMOB(tile, pos) ((flags_bin[tile] & 3) || mobmap[pos])

#define URAND() ((u8)rand())

#define POS_TO_ADDR(pos) (0x9822 + (((pos)&0xf0) << 1) + ((pos)&0xf))

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

#define MASK_UL  0b11111110
#define MASK_DL  0b11111101
#define MASK_DR  0b11111011
#define MASK_UR  0b11110111
#define MASK_D   0b11101111
#define MASK_U   0b11011111
#define MASK_R   0b10111111
#define MASK_L   0b01111111

#define DIR_UL(pos)   ((u8)(pos + 239))
#define DIR_U(pos)    ((u8)(pos + 240))
#define DIR_UR(pos)   ((u8)(pos + 241))
#define DIR_L(pos)    ((u8)(pos + 255))
#define DIR_R(pos)    ((u8)(pos + 1))
#define DIR_DL(pos)   ((u8)(pos + 15))
#define DIR_D(pos)    ((u8)(pos + 16))
#define DIR_DR(pos)   ((u8)(pos + 17))

#define TILE_EMPTY 1
#define TILE_WALL 2
#define TILE_WALL_FACE 3
#define TILE_WALL_FACE_PLANT 5
#define TILE_TELEPORTER 6
#define TILE_CARPET 7
#define TILE_VOID1 9
#define TILE_VOID2 10
#define TILE_END 0xd
#define TILE_START 0xe
#define TILE_GOAL 0x3e
#define TILE_TORCH_LEFT 0x3f
#define TILE_TORCH_RIGHT 0x41
#define TILE_HEART 0x42
#define TILE_DIRT1 0x43
#define TILE_DIRT2 0x44
#define TILE_DIRT3 0x48
#define TILE_PLANT1 0xb
#define TILE_PLANT2 0xc
#define TILE_PLANT3 0x47
#define TILE_FIXED_WALL 0x4a
#define TILE_SAW1 0x4e
#define TILE_WALL_FACE_CRACKED 0x60
#define TILE_STEPS 0x6a
#define TILE_ENTRANCE 0x6b

const u8 dirpos[]   = {0xff, 1, 0xf0, 16};  // L R U D
const u8 dirvalid[] = {VALID_L,VALID_R,VALID_U,VALID_D};

const u8 carvesig[] = {255, 223, 127, 191, 239, 0};
const u8 carvemask[] = {0, 9, 3, 12, 6};
const u8 doorsig[] = {207, 63, 0};
const u8 doormask[] = {15, 15};

const u8 freesig[] = {8, 4, 2, 1, 22, 76, 41, 131, 171, 109, 94, 151, 0};
const u8 freemask[] = {8, 4, 2, 1, 6, 12, 9, 3, 10, 5, 10, 5};

const u8 wallsig[] = {
    251, 239, 253, 95,  159, 91,  31,  157, 115, 217, 241, 248,
    219, 189, 231, 123, 191, 15,  127, 111, 175, 79,  240, 143,
    230, 188, 242, 244, 119, 238, 190, 221, 247, 223, 254, 207,
    63,  103, 47,  174, 245, 250, 243, 249, 246, 252, 0,
};
const u8 wallmask[] = {
    0,  6,  0, 11, 13, 11, 15, 13, 3, 9,  0, 0, 9, 12, 6,  3,
    12, 15, 3, 7,  14, 15, 0,  15, 6, 12, 0, 0, 3, 6,  12, 9,
    0,  9,  0, 15, 15, 7,  15, 14, 0, 0,  0, 0, 0, 0,
};

// floor(35 / x) for x in [3,10]
const u8 other_dim[] = {11, 8, 7, 5, 5, 4, 3, 3};

// 54  0101 0100 UL
// d6  1101 0110 U
// 92  1001 0010 UR
// 7c  0111 1100 L
// ff  1111 1111 M
// b3  1011 0011 R
// 68  0110 1000 DL
// e9  1110 1001 D
// a1  1010 0001 DR
const u8 validmap[] = {
  0x54,0xd6,0xd6,0xd6,0xd6,0xd6,0xd6,0xd6,0xd6,0xd6,0xd6,0xd6,0xd6,0xd6,0xd6,0x92,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb3,
  0x68,0xe9,0xe9,0xe9,0xe9,0xe9,0xe9,0xe9,0xe9,0xe9,0xe9,0xe9,0xe9,0xe9,0xe9,0xa1,
};

const u8 void_tiles[] = {TILE_VOID1, TILE_VOID1, TILE_VOID1, TILE_VOID1,
                         TILE_VOID1, TILE_VOID1, TILE_VOID2};
const u8 dirt_tiles[] = {TILE_EMPTY, TILE_DIRT1, TILE_DIRT2, TILE_DIRT3};
const u8 plant_tiles[] = {TILE_PLANT1, TILE_PLANT2, TILE_PLANT3};

const u8 room_permutations[][MAX_ROOMS] = {
    {0, 1, 2, 3}, {0, 1, 3, 2}, {0, 2, 1, 3}, {0, 2, 3, 1}, {0, 3, 1, 2},
    {0, 3, 2, 1}, {1, 0, 2, 3}, {1, 0, 3, 2}, {1, 2, 0, 3}, {1, 2, 3, 0},
    {1, 3, 0, 2}, {1, 3, 2, 0}, {2, 0, 1, 3}, {2, 0, 3, 1}, {2, 1, 0, 3},
    {2, 1, 3, 0}, {2, 3, 0, 1}, {2, 3, 1, 0}, {3, 0, 1, 2}, {3, 0, 2, 1},
    {3, 1, 0, 2}, {3, 1, 2, 0}, {3, 2, 0, 1}, {3, 2, 1, 0},
};

const u8 vase_mobs[] = {0, 0, 0, 0, 0, 0, MOB_TYPE_VASE1, MOB_TYPE_VASE2};
const u8 plant_mobs[] = {MOB_TYPE_WEED, MOB_TYPE_WEED, MOB_TYPE_BOMB};

const u8 mob_plan[] = {
    // floor 1
    MOB_TYPE_SLIME, MOB_TYPE_SLIME, MOB_TYPE_SLIME, MOB_TYPE_SLIME,
    MOB_TYPE_SLIME,
    // floor 2
    MOB_TYPE_SLIME, MOB_TYPE_SLIME, MOB_TYPE_QUEEN, MOB_TYPE_SLIME,
    MOB_TYPE_SLIME, MOB_TYPE_QUEEN,
    // floor 3
    MOB_TYPE_SLIME, MOB_TYPE_SLIME, MOB_TYPE_QUEEN, MOB_TYPE_SLIME,
    MOB_TYPE_SLIME, MOB_TYPE_QUEEN, MOB_TYPE_SLIME,
    // floor 4
    MOB_TYPE_SCORPION, MOB_TYPE_SCORPION, MOB_TYPE_SCORPION, MOB_TYPE_HULK,
    MOB_TYPE_SCORPION, MOB_TYPE_SCORPION, MOB_TYPE_SCORPION, MOB_TYPE_HULK,
    // floor 5
    MOB_TYPE_SCORPION, MOB_TYPE_SCORPION, MOB_TYPE_HULK, MOB_TYPE_SCORPION,
    MOB_TYPE_SCORPION, MOB_TYPE_HULK, MOB_TYPE_SCORPION, MOB_TYPE_SCORPION,
    MOB_TYPE_HULK,
    // floor 6
    MOB_TYPE_HULK, MOB_TYPE_SCORPION, MOB_TYPE_HULK, MOB_TYPE_SCORPION,
    MOB_TYPE_SCORPION, MOB_TYPE_HULK, MOB_TYPE_SCORPION, MOB_TYPE_SCORPION,
    MOB_TYPE_HULK,
    // floor 7
    MOB_TYPE_GHOST, MOB_TYPE_GHOST, MOB_TYPE_GHOST, MOB_TYPE_KONG,
    MOB_TYPE_GHOST, MOB_TYPE_GHOST, MOB_TYPE_GHOST, MOB_TYPE_KONG,
    MOB_TYPE_GHOST,
    // floor 8
    MOB_TYPE_GHOST, MOB_TYPE_GHOST, MOB_TYPE_KONG, MOB_TYPE_GHOST,
    MOB_TYPE_GHOST, MOB_TYPE_KONG, MOB_TYPE_GHOST, MOB_TYPE_GHOST,
    MOB_TYPE_KONG,
    // floor 9
    MOB_TYPE_GHOST, MOB_TYPE_KONG, MOB_TYPE_GHOST, MOB_TYPE_KONG,
    MOB_TYPE_GHOST, MOB_TYPE_KONG, MOB_TYPE_GHOST, MOB_TYPE_KONG,
    MOB_TYPE_GHOST};

const u8 mob_plan_floor[] = {
    0,  // floor 0
    0,  // floor 1
    5,  // floor 2
    11, // floor 3
    18, // floor 4
    26, // floor 5
    35, // floor 6
    44, // floor 7
    53, // floor 8
    62, // floor 9
    70, // total
};

const u8 mob_anim_frames[] = {
    0x99, 0x9a, 0x9b, 0x9c,                   // player
    0x80, 0x81, 0x80, 0x82,                   // slime
    0x86, 0x87, 0x88, 0x89,                   // queen
    0x8a, 0x8b, 0x8a, 0x8c,                   // scorpion
    0x83, 0x84, 0x83, 0x85,                   // hulk
    0x8d, 0x8e, 0x8f, 0x8e,                   // ghost
    0x90, 0x91, 0x92, 0x93,                   // kong
    0x9d, 0x9e, 0x9f, 0xa0,                   // reaper
    0x94, 0x95, 0x96, 0x95,                   // weed
    0x97, 0x97, 0x97, 0x97, 0x97, 0x97, 0x97, //
    0x97, 0x97, 0x97, 0x97, 0x97, 0x98,       // bomb
    0x59,                                     // vase1
    0x5f,                                     // vase2
    0x53,                                     // chest
};

const u8 mob_anim_start[] = {
    0,  // player
    4,  // slime
    8,  // queen
    12, // scorpion
    16, // hulk
    20, // ghost
    24, // kong
    28, // reaper
    32, // weed
    36, // bomb
    49, // vase1
    50, // vase2
    51, // chest
    52, // total
};

const u8 mob_anim_speed[] = {
    24, // player
    12, // slime
    12, // queen
    12, // scorpion
    13, // hulk
    16, // ghost
    12, // kong
    16, // reaper
    16, // weed
    8,  // bomb
    255, // vase1
    255, // vase2
    255, // chest
};

typedef u8 Map[MAP_WIDTH * MAP_HEIGHT];

typedef enum RoomKind {
  ROOM_KIND_VASE,
  ROOM_KIND_DIRT,
  ROOM_KIND_CARPET,
  ROOM_KIND_TORCH,
  ROOM_KIND_PLANT,
  NUM_ROOM_KINDS,
} RoomKind;

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
  NUM_MOB_TYPES,
} MobType;

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
} PickupType;

void init(void);
void animate_tiles(void);
void vbl_interrupt(void);

void addmob(MobType type, u8 pos);
void addpickup(PickupType type, u8 pos);

// Map generation
void mapgen(void);
void mapgeninit(void);
void copymap(u8 index);
void roomgen(void);
void mazeworms(void);
void update_carve1(u8 pos);
u8 nexttoroom4(u8 pos, u8 valid);
u8 nexttoroom8(u8 pos, u8 valid);
void digworm(u8 pos);
void carvedoors(void);
void update_door1(u8 pos);
void carvecuts(void);
void calcdist(u8 pos);
void startend(void);
u8 startscore(u8 pos);
void fillends(void);
void update_fill1(u8 pos);
void prettywalls(void);
void voids(void);
void chests(void);
void decoration(void);
void begintileanim(void);
void endtileanim(void);
void addtileanim(u8 pos, u8 tile);
void spawnmobs(void);
u8 no_mob_neighbors(u8 pos);
void mapset(u8* pos, u8 w, u8 h, u8 val);
void sigrect_empty(u8 pos, u8 w, u8 h);
void sigempty(u8 pos);
void sigwall(u8 pos);
u8 sigmatch(u16 pos, u8* sig, u8* mask);
u8 ds_union(u8 x, u8 y);
u8 ds_find(u8 id);
u8 getpos(u8 cand);
u8 randint(u8 mx);


Map tmap;    // Tile map
Map roommap; // Room/void map
Map distmap; // Distance map
Map mobmap;  // Mob map
Map sigmap;  // Signature map (tracks neighbors) **only used during mapgen
Map tempmap; // Temp map (used for carving)      **only used during mapgen

// The following data structures is used for rooms as well as union-find
// algorithm; see https://en.wikipedia.org/wiki/Disjoint-set_data_structure
Map ds_sets;                   // **only used during mapgen
u8 ds_parents[MAX_DS_SET];     // **only used during mapgen
u8 ds_sizes[MAX_DS_SET];       // **only used during mapgen
u8 ds_nextid;                  // **only used during mapgen
u8 ds_num_sets;                // **only used during mapgen
u8 dogate;                     // **only used during mapgen
u8 endpos, gatepos, heartpos;  // **only used during mapgen

u8 room_pos[MAX_ROOMS];
u8 room_w[MAX_ROOMS];
u8 room_h[MAX_ROOMS];
u8 room_avoid[MAX_ROOMS]; // ** only used during mapgen
u8 num_rooms;

u8 void_num_tiles[MAX_VOIDS]; // Number of tiles in a void region
u8 void_num_walls[MAX_VOIDS]; // Number of walls in a void region
u8 num_voids;

u8 start_room;
u8 floor;
u8 startpos;

u8 cands[MAX_CANDS];
u8 num_cands;

u8 mob_anim_frame[MAX_MOBS];
u8 mob_anim_timer[MAX_MOBS];
MobType mob_type[MAX_MOBS];
u8 mob_pos[MAX_MOBS];
u8 num_mobs;

// TODO: how big to make these arrays?
u8 tile_code[128];     // code for animating tiles
u8 mob_tile_code[256]; // code for animating mobs
u8 tile_code_end;      // next available index in tile_code
u16 last_tile_addr;    // last tile address in HL for {mob_,}tile_code
u8 last_tile_val;      // last tile value in A for {mob_,}tile_code
u8 tile_timer; // timer for animating tiles (every TILE_ANIM_SPEED frames)
u8 tile_inc;   // either 0 or TILE_ANIM_FRAME_DIFF, for two frames of animation

u8 key;

u16 myclock;
void tim_interrupt(void) { ++myclock; }

void main(void) {
  init();
  mapgen();

  set_bkg_tiles(2, 1, MAP_WIDTH, MAP_HEIGHT, tmap);
  LCDC_REG = 0b10000001;  // display on, bg on

  while(1) {
    key = joypad();

    // XXX
    if (key & J_A) {
      ++floor;
      if (floor == 11) { floor = 0; }
      DISPLAY_OFF;
      mapgen();
      set_bkg_tiles(2, 1, MAP_WIDTH, MAP_HEIGHT, tmap);
      LCDC_REG = 0b10000001;  // display on, bg on
    }

    animate_tiles();
    wait_vbl_done();
  }
}

void init(void) {
  DISPLAY_OFF;

   // 0:LightGray 1:DarkGray 2:Black 3:White
  BGP_REG = OBP0_REG = OBP1_REG = 0b00111001;

  add_TIM(tim_interrupt);
  TMA_REG = 0;      // Divide clock by 256 => 16Hz
  TAC_REG = 0b100;  // 4096Hz, timer on.
  IE_REG |= TIM_IFLAG;

  enable_interrupts();

  floor = 0;
  initrand(0x4321);  // TODO: seed with DIV on button press

  set_sprite_data(0, SPRITES_TILES, sprites_bin);
  set_bkg_data(0, BG_TILES, bg_bin);
  set_bkg_data(0x80, SHARED_TILES, shared_bin);
  init_bkg(0);

  num_mobs = 0;
  addmob(MOB_TYPE_PLAYER, 0);

  tile_inc = 0;
  tile_timer = TILE_ANIM_SPEED;
  tile_code[0] = mob_tile_code[0] = 0xc9; // ret
  add_VBL(vbl_interrupt);
}

void animate_tiles(void) {
  u8 first, i, frame;
  u8* ptr;
  u16 addr;

  first = 1;
  mob_tile_code[0] = 0xf5; // push af
  ptr = mob_tile_code + 1;

  // Loop through all mobs, update animations
  for (i = 0; i < num_mobs; ++i) {
    // TODO: mob must be drawn if it is on top of an animating tile, and it
    // updated this frame.
    if (--mob_anim_timer[i] == 0) {
      mob_anim_timer[i] = mob_anim_speed[mob_type[i]];
      if (++mob_anim_frame[i] == mob_anim_start[mob_type[i] + 1]) {
        mob_anim_frame[i] = mob_anim_start[mob_type[i]];
      }

      addr = POS_TO_ADDR(mob_pos[i]);
      frame = mob_anim_frames[mob_anim_frame[i]];

      if (first) {
        first = 0;
        *ptr++ = 0x21; // ld hl, addr
        *ptr++ = (u8)addr;
        *ptr++ = addr >> 8;
        *ptr++ = 0x3e; // ld a, frame
        *ptr++ = frame;
      } else {
        if ((addr >> 8) == (last_tile_addr >> 8)) {
          *ptr++ = 0x2e; // ld l, lo(addr)
          *ptr++ = (u8)addr;
        } else {
          *ptr++ = 0x21; // ld hl, addr
          *ptr++ = (u8)addr;
          *ptr++ = addr >> 8;
        }
        if (frame != last_tile_val) {
          *ptr++ = 0x3e; // ld a, frame
          *ptr++ = frame;
        }
      }
      last_tile_addr = addr;
      last_tile_val = frame;
      *ptr++ = 0x77; // ld (hl), a
    }
  }
  *ptr++ = 0xf1; // pop af
  *ptr++ = 0xc9; // ret
}

void vbl_interrupt(void) {
  if (--tile_timer == 0) {
    tile_timer = TILE_ANIM_SPEED;
    tile_inc ^= TILE_ANIM_FRAME_DIFF;
    ((vfp)tile_code)();
  }

  ((vfp)mob_tile_code)();
}

u8 dummy[13];
#define TIMESTART dummy[0] = (u8)(myclock)
#define TIMEDIFF(x)                                                            \
  dummy[x] = (u8)(myclock)-dummy[0];                                           \
  TIMESTART

void addmob(MobType type, u8 pos) {
  mob_type[num_mobs] = type;
  mob_pos[num_mobs] = pos;
  mob_anim_timer[num_mobs] = 1;
  mob_anim_frame[num_mobs] = mob_anim_start[type];
  ++num_mobs;
  mobmap[pos] = num_mobs; // index+1
}

void addpickup(PickupType pick, u8 pos) {
  // TODO
  (void)pick;
  (void)pos;
}

void mapgen(void) {
  num_mobs = 1;
  mapgeninit();
  begintileanim();
  if (floor == 0) {
    copymap(0);
    addmob(MOB_TYPE_CHEST, 119);  // x=7,y=7
  } else if (floor == 10) {
    copymap(1);
  } else {
    TIMESTART;
    roomgen();
    TIMEDIFF(1);
    mazeworms();
    TIMEDIFF(2);
    carvedoors();
    TIMEDIFF(3);
    carvecuts();
    TIMEDIFF(4);
    startend();
    TIMEDIFF(5);
    fillends();
    TIMEDIFF(6);
    prettywalls();
    TIMEDIFF(7);
    voids();
    TIMEDIFF(8);
    chests();
    TIMEDIFF(9);
    decoration();
    TIMEDIFF(10);
    spawnmobs();
    TIMEDIFF(11);

    dummy[12] = dummy[1] + dummy[2] + dummy[3] + dummy[4] + dummy[5] +
                dummy[6] + dummy[7] + dummy[8] + dummy[9] + dummy[10] + dummy[11];
  }
  endtileanim();
}

void mapgeninit(void) {
  memset(roommap, 0, sizeof(roommap));
  memset(mobmap, 0, sizeof(mobmap));
  memset(sigmap, 255, sizeof(sigmap));
  memset(ds_sets, 0, sizeof(ds_sets));

  dogate = 0;

  // Initialize disjoint set
  ds_nextid = 0;
  ds_parents[0] = 0;
  ds_num_sets = 0;

  num_rooms = 0;
  num_voids = 0;
  start_room = 0;
}

void roomgen(void) {
  u8 failmax = 5, maxwidth = 10, maxheight = 10, maxh;
  u8 sig, valid, walkable, val, w, h, pos;

  dogate = floor == 3 || floor == 6 || floor == 9;

  if (dogate) {
    // Pick a random gate map
    copymap(2 + randint(14));
  } else {
    // Start with a completely empty map (all walls)
    memset(tmap, TILE_WALL, sizeof(tmap));
  }

  do {
    // Pick width and height for room.
    w = 3 + randint(maxwidth - 2);
    maxh = other_dim[w - 3];
    if (maxh > maxheight) maxh = maxheight;
    h = 3 + randint(maxh - 2);

    // Update width/height maps
    //
    // The basic idea is to start in the lower-right corner and work backward.
    // There are two maps; one that tracks whether a room of this width will
    // fit, and the other tracks whether a room of this height will fit. When
    // both values are 0, the room fits.
    //
    // The room must have a one tile border around it, except for when it is
    // adjacent to the map edge.
    num_cands = 0;
    pos = 255;
    do {
      valid = validmap[pos];
      sig = ~sigmap[pos];
      walkable = !IS_WALL_TILE(tmap[pos]);
      if (walkable || (sig & VALID_L_OR_UL)) {
        // This tile is either in a room, or on the right or lower border of
        // the room.
        val = h + 1;
      } else if (valid & VALID_D) {
        // This tile is above a room or the bottom edge; we subtract one from
        // the value below to track whether it is far enough away to fit a room
        // of height `h`.
        val = tempmap[DIR_D(pos)];
        if (val) { --val; }
      } else {
        // This tile is on the bottom edge of the map.
        val = h - 1;
      }
      tempmap[pos] = val;

      if (walkable || (sig & VALID_U_OR_UL)) {
        // This tile is either in a room, or on the right or lower border of
        // the room.
        val = w + 1;
      } else if (valid & VALID_R) {
        val = distmap[DIR_R(pos)];
        if (val) {
          // This tile is to the left of a room or the right edge of the map.
          // We also update the "height" map in this case to extend out the
          // invalid region vertically. For example, if placing a 3x3 room,
          // where there is an existing room (marked with x):
          //
          // width map   height map   overlap
          // 000000012   000000000    0000000..
          // 000000012   011112220    0........
          // 000000012   022222220    0........
          // 000000012   033333330    0........
          // 0123xxxx2   0444xxxx0    0........
          // 0123xxxx2   0444xxxx0    0........
          // 0123xxxx2   1111xxxx1    .........
          // 000000012   222222222    .........
          //
          --val;
          tempmap[pos] = tempmap[DIR_R(pos)];
        } else {
          val = 0;
          if (!tempmap[pos]) {
            // This is valid position, add it to the candidate list.
            cands[num_cands++] = pos;
          }
        }
      } else {
        val = w - 1;
      }
      distmap[pos] = val;
    } while(pos--);

    if (num_cands) {
      pos = cands[randint(num_cands)];

      room_pos[num_rooms] = pos;
      room_w[num_rooms] = w;
      room_h[num_rooms] = h;
      room_avoid[num_rooms] = 0;

      // Fill room tiles at x, y
      mapset(tmap + pos, w, h, 1);
      mapset(roommap + pos, w, h, num_rooms + 1);
      sigrect_empty(pos, w, h);

      // Update disjoint set regions; we know that they're disjoint so they
      // each get their own region id (i.e. the room number)
      mapset(ds_sets + pos, w, h, ++ds_nextid);
      ds_parents[ds_nextid] = ds_nextid;
      ds_sizes[ds_nextid] = w * h; // XXX multiply
      ++ds_num_sets;

      if (++num_rooms == 1) {
        maxwidth >>= 1;
        maxheight >>= 1;
      }
    } else {
      // cannot place room
      --failmax;
    }
  } while (failmax && num_rooms < MAX_ROOMS);
}

void copymap(u8 index) {
  u8 pos, valid;

  memcpy(tmap, map_bin + (index << 8), 256);

  // Update signature map for empty tiles
  pos = 0;
  do {
    switch (tmap[pos]) {
      case TILE_END:
        endpos = pos;
        goto empty;

      case TILE_START:
      case TILE_ENTRANCE:
        startpos = pos;
        mobmap[mob_pos[PLAYER_MOB]] = 0; // Clear old player location
        mob_pos[PLAYER_MOB] = pos;
        mobmap[pos] = PLAYER_MOB + 1;
        goto empty;

      case TILE_GOAL:
        // Temporarily change to stairs so the empty spaces behind the gate
        // can be part of the spanning graph created during the carvedoors()
        // step.
        tmap[pos] = TILE_END;
        gatepos = pos;
        goto empty;

      case TILE_TORCH_LEFT:
      case TILE_TORCH_RIGHT:
        addtileanim(pos, tmap[pos]);
        goto empty;

      case TILE_HEART:
        heartpos = pos;
        goto empty;

      empty:
      case TILE_CARPET:
      case TILE_EMPTY:
      case TILE_STEPS:
        // Update sigmap
        sigempty(pos);

        // Update disjoint set for the gate area
        ++ds_nextid;
        ds_sets[pos] = ds_nextid;
        ds_parents[ds_nextid] = ds_nextid;
        ds_sizes[ds_nextid] = 1;

        valid = validmap[pos];
        if (valid & VALID_U) { ds_union(ds_nextid, ds_sets[DIR_U(pos)]); }
        if (valid & VALID_L) { ds_union(ds_nextid, ds_sets[DIR_L(pos)]); }
        if (valid & VALID_R) { ds_union(ds_nextid, ds_sets[DIR_R(pos)]); }
        if (valid & VALID_D) { ds_union(ds_nextid, ds_sets[DIR_D(pos)]); }
        break;
    }
  } while (++pos);
}

void mazeworms(void) {
  u8 pos, cand, valid;

  memset(tempmap, 0, sizeof(tempmap));

  // Find all candidates for worm start position
  num_cands = 0;
  pos = 0;
  do {
    update_carve1(pos);
  } while(++pos);

  do {
    if (num_cands) {
      // Choose a random start, and calculate its offset
      cand = randint(num_cands);
      pos = 0;
      do {
        if (tempmap[pos] == 2 && cand-- == 0) { break; }
      } while(++pos);

      // Update the disjoint set with this new region.
      ++ds_nextid;
      ++ds_num_sets;
      ds_parents[ds_nextid] = ds_nextid;
      ds_sizes[ds_nextid] = 0;

      digworm(pos);

      // Only the first spot may be connected to another empty tile. No other
      // tile can be connect to another region (since it would not be
      // carvable). So we only need to merge regions here.
      valid = validmap[pos];
      if (valid & VALID_U) { ds_union(ds_nextid, ds_sets[DIR_U(pos)]); }
      if (valid & VALID_L) { ds_union(ds_nextid, ds_sets[DIR_L(pos)]); }
      if (valid & VALID_R) { ds_union(ds_nextid, ds_sets[DIR_R(pos)]); }
      if (valid & VALID_D) { ds_union(ds_nextid, ds_sets[DIR_D(pos)]); }
    }
  } while(num_cands > 1);
}

void update_carve1(u8 pos) {
  u8 tile = tmap[pos];
  u8 result;
  if (tile != TILE_FIXED_WALL && IS_WALL_TILE(tile) && CAN_CARVE(pos)) {
    result = 1;
    if (!nexttoroom4(pos, validmap[pos])) {
      ++result;
      if (tempmap[pos] != 2) { ++num_cands; }
    }
  } else {
    result = 0;
    if (tempmap[pos] == 2) { --num_cands; }
  }
  tempmap[pos] = result;
}

u8 nexttoroom4(u8 pos, u8 valid) {
  return ((valid & VALID_U) && roommap[DIR_U(pos)]) ||
         ((valid & VALID_L) && roommap[DIR_L(pos)]) ||
         ((valid & VALID_R) && roommap[DIR_R(pos)]) ||
         ((valid & VALID_D) && roommap[DIR_D(pos)]);
}

u8 nexttoroom8(u8 pos, u8 valid) {
  return nexttoroom4(pos, valid) ||
         ((valid & VALID_UL) && roommap[DIR_UL(pos)]) ||
         ((valid & VALID_UR) && roommap[DIR_UR(pos)]) ||
         ((valid & VALID_DL) && roommap[DIR_DL(pos)]) ||
         ((valid & VALID_DR) && roommap[DIR_DR(pos)]);
}

void digworm(u8 pos) {
  u8 dir, step, num_dirs, valid;

  // Pick a random direction
  dir = URAND() & 3;
  step = 0;
  do {
    tmap[pos] = 1; // Set tile to empty.
    sigempty(pos); // Update neighbor signatures.

    // Remove this tile from the carvable map
    if (tempmap[pos] == 2) { --num_cands; }
    tempmap[pos] = 0;

    // Update neighbors
    valid = validmap[pos];
    if (valid & VALID_UL) { update_carve1(DIR_UL(pos)); }
    if (valid & VALID_U)  { update_carve1(DIR_U (pos)); }
    if (valid & VALID_UR) { update_carve1(DIR_UR(pos)); }
    if (valid & VALID_L)  { update_carve1(DIR_L (pos)); }
    if (valid & VALID_R)  { update_carve1(DIR_R (pos)); }
    if (valid & VALID_DL) { update_carve1(DIR_DL(pos)); }
    if (valid & VALID_D)  { update_carve1(DIR_D (pos)); }
    if (valid & VALID_DR) { update_carve1(DIR_DR(pos)); }

    // Update the disjoint set
    ds_sets[pos] = ds_nextid;
    ++ds_sizes[ds_nextid];

    // Continue in the current direction, unless we can't carve. Also randomly
    // change direction after 3 or more steps.
    if (!((valid & dirvalid[dir]) && tempmap[(u8)(pos + dirpos[dir])]) ||
        ((URAND() & 1) && step > 2)) {
      step = 0;
      num_dirs = 0;
      if ((valid & VALID_L) && tempmap[DIR_L(pos)]) { cands[num_dirs++] = 0; }
      if ((valid & VALID_R) && tempmap[DIR_R(pos)]) { cands[num_dirs++] = 1; }
      if ((valid & VALID_U) && tempmap[DIR_U(pos)]) { cands[num_dirs++] = 2; }
      if ((valid & VALID_D) && tempmap[DIR_D(pos)]) { cands[num_dirs++] = 3; }
      if (num_dirs == 0) return;
      dir = cands[randint(num_dirs)];
    }
    pos += dirpos[dir];
    ++step;
  } while(1);
}

void carvedoors(void) {
  u8 pos, match, diff;

  memset(tempmap, 0, sizeof(tempmap));

  pos = 0;
  num_cands = 0;
  do {
    update_door1(pos);
  } while(++pos);

  do {
    // Pick a random door, and calculate its position.
    pos = getpos(randint(num_cands));

    // Merge the regions, if possible. They may be already part of the same
    // set, in which case they should not be joined.
    match = IS_DOOR(pos);
    diff = match == 1 ? 16 : 1; // 1: horizontal, 2: vertical
    if (ds_union(ds_sets[pos - diff], ds_sets[pos + diff])) {
      // Insert an empty tile to connect the regions
      ds_sets[pos] = ds_sets[pos + diff];
      tmap[pos] = 1;
      sigempty(pos); // Update neighbor signatures.

      // Update neighbors
      u8 valid = validmap[pos];
      if (valid & VALID_U) { update_door1(DIR_U(pos)); }
      if (valid & VALID_L) { update_door1(DIR_L(pos)); }
      if (valid & VALID_R) { update_door1(DIR_R(pos)); }
      if (valid & VALID_D) { update_door1(DIR_D(pos)); }
    }

    // Remove this tile from the carvable map
    --num_cands;
    tempmap[pos] = 0;
  } while (num_cands);

  // TODO: its possible to generate a map that is not fully-connected; in that
  // case we need to start over.
}

void update_door1(u8 pos) {
  // A door connects two regions in the following patterns:
  //   * 0 *   * 1 *
  //   1   1   0   0
  //   * 0 *   * 1 *
  //
  // Where 1s are walls and 0s are not. The `match` index returns whether it is
  // a horizontal door or vertical. A door is only allowed between two regions
  // that are not already connected. We use the disjoint set to determine this
  // quickly below.
  u8 tile = tmap[pos];
  u8 result = tile != TILE_FIXED_WALL && IS_WALL_TILE(tile) && IS_DOOR(pos);
  if (tempmap[pos] != result) {
    if (result) {
      ++num_cands;
    } else {
      --num_cands;
    }
    tempmap[pos] = result;
  }
}

void carvecuts(void) {
  u8 pos, diff, match;
  u8 maxcuts = 3;

  num_cands = 0;
  pos = 0;
  do {
    update_door1(pos);
  } while(++pos);

  do {
    // Pick a random cut, and calculate its position.
    pos = getpos(randint(num_cands));

    // Calculate distance from one side of the door to the other.
    match = IS_DOOR(pos);
    diff = match == 1 ? 16 : 1; // 1: horizontal, 2: vertical
    // TODO: we only need the distance to the other size of the door; use a
    // faster routine for this?
    calcdist(pos + diff);

    // Remove this candidate
    tempmap[pos] = 0;
    --num_cands;

    // Connect the regions (creating a cycle) if the distance between the two
    // sides is > 20 steps (21 is used because the distance starts at 1; that's
    // so that 0 can be used to represent an unvisited cell).
    if (distmap[(u8)(pos - diff)] > 21) {
      tmap[pos] = 1;
      sigempty(pos); // Update neighbor signatures.
      --maxcuts;
    }
  } while (num_cands && maxcuts);
}

void calcdist(u8 pos) {
  u8 head, oldtail, newtail, sig, valid, dist, newpos;
  cands[head = 0] = pos;
  newtail = 1;

  memset(distmap, 0, sizeof(distmap));
  distmap[pos] = dist = 1;

  do {
    oldtail = newtail;
    ++dist;
    do {
      pos = cands[head++];
      valid = validmap[pos];
      sig = ~sigmap[pos] & valid;
      if (!distmap[newpos = DIR_U(pos)]) {
        if (valid & VALID_U) { distmap[newpos] = dist; }
        if (sig & VALID_U) { cands[newtail++] = newpos; }
      }
      if (!distmap[newpos = DIR_L(pos)]) {
        if (valid & VALID_L) { distmap[newpos] = dist; }
        if (sig & VALID_L) { cands[newtail++] = newpos; }
      }
      if (!distmap[newpos = DIR_R(pos)]) {
        if (valid & VALID_R) { distmap[newpos] = dist; }
        if (sig & VALID_R) { cands[newtail++] = newpos; }
      }
      if (!distmap[newpos = DIR_D(pos)]) {
        if (valid & VALID_D) { distmap[newpos] = dist; }
        if (sig & VALID_D) { cands[newtail++] = newpos; }
      }
    } while (head != oldtail);
  } while (oldtail != newtail);
}

void startend(void) {
  u8 pos, best, score;

  if (!dogate) {
    // Pick a random walkable location
    do {
      endpos = URAND();
    } while (IS_WALL_TILE(tmap[endpos]));
  }

  calcdist(endpos);

  startpos = 0;
  best = 0;
  // Find the furthest point from endpos.
  pos = 0;
  do {
    if (distmap[pos] > best) {
      startpos = pos;
      best = distmap[pos];
    }
  } while(++pos);

  calcdist(startpos);

  if (!dogate) {
    // Now pick the furthest, freestanding location in a room from the startpos.
    pos = 0;
    best = 0;
    do {
      if (distmap[pos] > best && roommap[pos] && IS_FREESTANDING(pos)) {
        endpos = pos;
        best = distmap[pos];
      }
    } while(++pos);
  }

  // Finally pick the closest point that has the best startscore.
  pos = 0;
  best = 255;
  do {
    score = startscore(pos);
    if (distmap[pos] && score) {
      score = distmap[pos] + score;
      if (score < best) {
        best = score;
        startpos = pos;
      }
    }
  } while(++pos);

  if (roommap[startpos]) {
    start_room = roommap[startpos];
  }

  tmap[startpos] = TILE_START;
  tmap[endpos] = TILE_END;
  sigempty(startpos);
  sigempty(endpos);
  mob_pos[PLAYER_MOB] = startpos;
  mobmap[startpos] = PLAYER_MOB + 1;

  if (dogate) {
    tmap[gatepos] = TILE_GOAL;
    if (floor < 9) {
      addpickup(PICKUP_TYPE_HEART, heartpos);
    }
  }
}

u8 startscore(u8 pos) {
  u8 score = IS_FREESTANDING(pos);
  // If the position is not in a room...
  if (!roommap[pos]) {
    // ...and not next to a room...
    if (!nexttoroom8(pos, validmap[pos])) {
      if (score) {
        // ...and it's freestanding, then give the best score
        return 1;
      } else if (CAN_CARVE(pos)) {
        // If the position is carvable, give a low score
        return 6;
      }
    }
  } else if (score) {
    // otherwise the position is in a room, so give a good score if it is
    // "more" freestanding (not in a corner).
    return score <= 9 ? 3 : 6;
  }
  return 0; // Invalid start position.
}

void fillends(void) {
  u8 pos, valid;
  memset(tempmap, 0, sizeof(tempmap));

  // Find all starting positions (dead ends)
  num_cands = 0;
  pos = 0;
  do {
    update_fill1(pos);
  } while(++pos);

  do {
    if (num_cands) {
      // Choose a random point, and calculate its offset
      pos = getpos(randint(num_cands));

      tmap[pos] = TILE_WALL; // Set tile to wall
      sigwall(pos);          // Update neighbor signatures.

      // Remove this cell from the dead end map
      --num_cands;
      tempmap[pos] = 0;

      valid = validmap[pos];
      if (valid & VALID_UL) { update_fill1(DIR_UL(pos)); }
      if (valid & VALID_U)  { update_fill1(DIR_U (pos)); }
      if (valid & VALID_UR) { update_fill1(DIR_UR(pos)); }
      if (valid & VALID_L)  { update_fill1(DIR_L (pos)); }
      if (valid & VALID_R)  { update_fill1(DIR_R (pos)); }
      if (valid & VALID_DL) { update_fill1(DIR_DL(pos)); }
      if (valid & VALID_D)  { update_fill1(DIR_D (pos)); }
      if (valid & VALID_DR) { update_fill1(DIR_DR(pos)); }
    }
  } while (num_cands);
}

void update_fill1(u8 pos) {
  u8 tile = tmap[pos];
  u8 result = !IS_WALL_TILE(tmap[pos]) && CAN_CARVE(pos) &&
              tile != TILE_START && tile != TILE_END;
  if (tempmap[pos] != result) {
    if (result) {
      ++num_cands;
    } else {
      --num_cands;
    }
    tempmap[pos] = result;
  }
}

void prettywalls(void) {
  u8 pos, tile;
  pos = 0;
  do {
    if (tmap[pos] == TILE_FIXED_WALL) {
      tmap[pos] = TILE_WALL;
    }
    if (tmap[pos] == TILE_WALL) {
      tile = sigmatch(pos, wallsig, wallmask);
      if (tile) {
        tile += 14;
      }
      if (TILE_HAS_CRACKED_VARIANT(tile) && !(URAND() & 7)) {
        tile += 0x35;
      }
      tmap[pos] = tile;
    } else if (tmap[pos] == TILE_EMPTY && (validmap[pos] & VALID_U) &&
               IS_WALL_TILE(tmap[DIR_U(pos)])) {
      tmap[pos] = IS_CRACKED_WALL_TILE(tmap[DIR_U(pos)])
                      ? TILE_WALL_FACE_CRACKED
                      : TILE_WALL_FACE;
    }
  } while(++pos);
}

void voids(void) {
  u8 pos, head, oldtail, newtail, valid, newpos;
  pos = 0;
  num_voids = 0;

  memset(void_num_tiles, 0, sizeof(void_num_tiles));

  do {
    if (!tmap[pos]) {
      // flood fill this region
      cands[head = 0] = pos;
      newtail = 1;
      do {
        oldtail = newtail;
        do {
          pos = cands[head++];
          if (!tmap[pos]) {
            roommap[pos] = num_rooms + num_voids + 1;
            ++void_num_tiles[num_voids];
            tmap[pos] = void_tiles[randint(sizeof(void_tiles))];
            valid = validmap[pos];

            // TODO: handle void exits
            if ((valid & VALID_U) && !tmap[newpos = DIR_U(pos)]) {
              cands[newtail++] = newpos;
            }
            if ((valid & VALID_L) && !tmap[newpos = DIR_L(pos)]) {
              cands[newtail++] = newpos;
            }
            if ((valid & VALID_R) && !tmap[newpos = DIR_R(pos)]) {
              cands[newtail++] = newpos;
            }
            if ((valid & VALID_D) && !tmap[newpos = DIR_D(pos)]) {
              cands[newtail++] = newpos;
            }
          }
        } while(head != oldtail);
      } while (oldtail != newtail);
      ++num_voids;
    }
  } while(++pos);
}

void chests(void) {
  u8 has_teleporter, room, chest_pos, i, pos, cand0, cand1;

  // Teleporter in level if level >= 5 and rnd(5)<1 (20%)
  has_teleporter = floor >= 5 && URAND() < 51;

  // Pick a random room
  room = randint(num_rooms);

  // Pick a random x,y in room (non-edge) for chest
  chest_pos;
  do {
    chest_pos = room_pos[room] + (((randint(room_h[room] - 2) + 1) << 4) |
                                  (randint(room_w[room] - 2) + 1));
  } while (tmap[chest_pos] != 1);

  // Pick a random position in each void and add it to the candidates. If
  // has_teleporter is true, then add chest_pos too.
  // The candidate indexes are stored starting at `num_voids`, so that the
  // actual positions can be stored starting at 0.
  for (i = 0; i < num_voids; ++i) {
    cands[1 + num_rooms + num_voids + i] = randint(void_num_tiles[i]);
  }

  // Convert these candidate to positions
  pos = 0;
  do {
    if (cands[num_voids + roommap[pos]]-- == 0) {
      // Subtract num_rooms so that the void regions starts at candidate 0 and
      // the rooms are not included.
      cands[(u8)(roommap[pos] - num_rooms - 1)] = pos;
    }
  } while (++pos);

  num_cands = num_voids;
  if (has_teleporter) {
    cands[num_cands++] = chest_pos;
  }

  if (num_cands > 2) {
    // Pick two random candidates and turn them into teleporters
    cand0 = randint(num_cands);
    cand1 = randint(num_cands - 1);
    if (cand1 == cand0) { ++cand1; }
    tmap[cands[cand0]] = tmap[cands[cand1]] = TILE_TELEPORTER;

    // Swap out the already used candidates
    cands[cand0] = cands[--num_cands];
    cands[cand1] = cands[--num_cands];
  }

  // Add chest position as a candidate if it wasn't already included above
  if (!has_teleporter) {
    cands[num_cands++] = chest_pos;
  }

  for (i = 0; i < num_cands; ++i) {
    addmob(MOB_TYPE_CHEST, cands[i]);
    // TODO: randomly make some chests heart containers
  }
}

void decoration(void) {
  RoomKind kind = ROOM_KIND_VASE;
  const u8 (*room_perm)[MAX_ROOMS] = room_permutations + randint(24);
  u8 i, room, pos, tile, w, h, mob;
  for (i = 0; i < MAX_ROOMS; ++i) {
    room = (*room_perm)[i];
    if (room >= num_rooms) { continue; }

    pos = room_pos[room];
    h = room_h[room];
    do {
      w = room_w[room];
      do {
        tile = tmap[pos];

        switch (kind) {
          case ROOM_KIND_VASE:
            mob = vase_mobs[randint(sizeof(vase_mobs))];
            if (mob && !IS_SMARTMOB(tile, pos) && IS_FREESTANDING(pos) &&
                (h == room_h[room] || h == 1 || w == room_w[room] || w == 1)) {
              addmob(mob, pos);
            }
            break;

          case ROOM_KIND_DIRT:
            if (tile == TILE_EMPTY) {
              tmap[pos] = dirt_tiles[URAND() & 3];
            }
            break;

          case ROOM_KIND_CARPET:
            if (tile == TILE_EMPTY && h != room_h[room] && h != 1 &&
                w != room_w[room] && w != 1) {
              tmap[pos] = TILE_CARPET;
            }
            // fallthrough

          case ROOM_KIND_TORCH:
            if (tile == TILE_EMPTY && (URAND() < 85) && (h & 1) &&
                IS_FREESTANDING(pos)) {
              if (w == 1) {
                tmap[pos] = TILE_TORCH_RIGHT;
                addtileanim(pos, TILE_TORCH_RIGHT);
              } else if (w == room_w[room]) {
                tmap[pos] = TILE_TORCH_LEFT;
                addtileanim(pos, TILE_TORCH_LEFT);
              }
            }
            break;

          case ROOM_KIND_PLANT:
            if (tile == TILE_EMPTY) {
              tmap[pos] = plant_tiles[randint(3)];
            } else if (tile == TILE_WALL_FACE) {
              tmap[pos] = TILE_WALL_FACE_PLANT;
            }

            if (!IS_SMARTMOB(tile, pos) && room + 1 != start_room &&
                !(URAND() & 3) && no_mob_neighbors(pos) &&
                IS_FREESTANDING(pos)) {
              mob = plant_mobs[randint(sizeof(plant_mobs))];
              addmob(mob, pos);
              if (mob == MOB_TYPE_WEED) {
                room_avoid[room] = 1;
              }
            }
            break;
        }

        if (kind != ROOM_KIND_PLANT && room + 1 != start_room &&
            tmap[pos] == TILE_EMPTY && !mobmap[pos] && IS_FREESTANDING(pos) &&
            URAND() < 26) {
          tmap[pos] = TILE_SAW1;
          sigwall(pos);
          addtileanim(pos, TILE_SAW1);
        }

        ++pos;
      } while(--w);
      pos += MAP_WIDTH - room_w[room];
    } while(--h);


    // Pick a room kind for the next room
    kind = randint(NUM_ROOM_KINDS);
  }
}

void begintileanim(void) {
  u16 addr = (u16)&tile_inc;
  tile_code[0] = 0xf5; // push af
  tile_code[1] = 0xc5; // push bc
  tile_code[2] = 0xfa; // ld a, (NNNN)
  tile_code[3] = (u8)addr;
  tile_code[4] = addr >> 8;
  tile_code[5] = 0x47; // ld b, a
  tile_code_end = 6;
}

void endtileanim(void) {
  tile_code[tile_code_end++] = 0xc1; // pop bc
  tile_code[tile_code_end++] = 0xf1; // pop af
  tile_code[tile_code_end] = 0xc9; // ret
}

void addtileanim(u8 pos, u8 tile) {
  u16 addr = POS_TO_ADDR(pos);
  if (tile_code_end == 6 || (addr >> 8) != (last_tile_addr >> 8)) {
    tile_code[tile_code_end++] = 0x21; // ld hl, NNNN
    tile_code[tile_code_end++] = (u8)addr;
    tile_code[tile_code_end++] = addr >> 8;
  } else {
    tile_code[tile_code_end++] = 0x2e; // ld l, NN
    tile_code[tile_code_end++] = (u8)addr;
  }
  if (tile != last_tile_val) {
    tile_code[tile_code_end++] = 0x3e; // ld a, NN
    tile_code[tile_code_end++] = tile;
    tile_code[tile_code_end++] = 0x80; // add b
  }
  tile_code[tile_code_end++] = 0x77; // ld (hl), a
  last_tile_addr = addr;
  last_tile_val = tile;
}

void spawnmobs(void) {
  u8 pos, room, mobs;

  pos = 0;
  num_cands = 0;
  do {
    room = roommap[pos];
    if (room && room != start_room && room <= num_rooms &&
        !IS_SMARTMOB(tmap[pos], pos) && !room_avoid[room - 1]) {
      tempmap[pos] = 1;
      ++num_cands;
    } else {
      tempmap[pos] = 0;
    }
  } while (++pos);

  mobs = mob_plan_floor[floor + 1] - mob_plan_floor[floor];

  do {
    pos = getpos(randint(num_cands));
    tempmap[pos] = 0;
    --num_cands;
    --mobs;
    addmob(mob_plan[mob_plan_floor[floor] + mobs], pos);
  } while (mobs && num_cands);

  // TODO: pack mobs into avoid rooms

  // TODO: give a mob a key, if this is a gate floor
}

u8 no_mob_neighbors(u8 pos) {
  u8 valid = validmap[pos];
  return !(((valid & VALID_U) && mobmap[DIR_U(pos)]) ||
           ((valid & VALID_L) && mobmap[DIR_L(pos)]) ||
           ((valid & VALID_R) && mobmap[DIR_R(pos)]) ||
           ((valid & VALID_D) && mobmap[DIR_D(pos)]));
}

void mapset(u8* pos, u8 w, u8 h, u8 val) {
  u8 w2 = w;
  do {
    *pos++ = val;
    if (--w2 == 0) {
      w2 = w;
      --h;
      pos += MAP_WIDTH - w;
    }
  } while (h);
}

void sigrect_empty(u8 pos, u8 w, u8 h) {
  u8 w2 = w;
  do {
    sigempty(pos);
    ++pos;

    if (--w2 == 0) {
      w2 = w;
      --h;
      pos += MAP_WIDTH - w;
    }
  } while (h);
}

void sigempty(u8 pos) {
  u8 valid = validmap[pos];
  if (valid & VALID_UL) { sigmap[DIR_UL(pos)] &= MASK_DR; }
  if (valid & VALID_U)  { sigmap[DIR_U (pos)] &= MASK_D; }
  if (valid & VALID_UR) { sigmap[DIR_UR(pos)] &= MASK_DL; }
  if (valid & VALID_L)  { sigmap[DIR_L (pos)] &= MASK_R; }
  if (valid & VALID_R)  { sigmap[DIR_R (pos)] &= MASK_L; }
  if (valid & VALID_DL) { sigmap[DIR_DL(pos)] &= MASK_UR; }
  if (valid & VALID_D)  { sigmap[DIR_D (pos)] &= MASK_U; }
  if (valid & VALID_DR) { sigmap[DIR_DR(pos)] &= MASK_UL; }
}

void sigwall(u8 pos) {
  u8 valid = validmap[pos];
  if (valid & VALID_UL) { sigmap[DIR_UL(pos)] |= VALID_DR; }
  if (valid & VALID_U)  { sigmap[DIR_U (pos)] |= VALID_D; }
  if (valid & VALID_UR) { sigmap[DIR_UR(pos)] |= VALID_DL; }
  if (valid & VALID_L)  { sigmap[DIR_L (pos)] |= VALID_R; }
  if (valid & VALID_R)  { sigmap[DIR_R (pos)] |= VALID_L; }
  if (valid & VALID_DL) { sigmap[DIR_DL(pos)] |= VALID_UR; }
  if (valid & VALID_D)  { sigmap[DIR_D (pos)] |= VALID_U; }
  if (valid & VALID_DR) { sigmap[DIR_DR(pos)] |= VALID_UL; }
}

u8 sigmatch(u16 pos, u8* sig, u8* mask) {
  u8 result = 0;
  u8 val = sigmap[pos];
  while (1) {
    if (*sig == 0) { return 0; }
    if ((val | *mask) == *sig) {
      return result + 1;
    }
    ++mask;
    ++sig;
    ++result;
  }
}

u8 ds_union(u8 x, u8 y) {
  if (!y) return 0;
  if (!x) return 0;  // TODO: remove this since x is always called w/ valid id
  x = ds_find(x);
  y = ds_find(y);

  if (x == y) return 0;

  if (ds_sizes[x] < ds_sizes[y]) {
    u8 t = x;
    x = y;
    y = t;
  }

  ds_parents[y] = x;
  ds_sizes[x] = ds_sizes[x] + ds_sizes[y];
  --ds_num_sets;
  return 1;
}

u8 ds_find(u8 x) {
  // Use path-halving as described in
  // https://en.wikipedia.org/wiki/Disjoint-set_data_structure
  while (x != ds_parents[x]) {
    x = ds_parents[x] = ds_parents[ds_parents[x]];
  }
  return x;
}

u8 getpos(u8 cand) {
  u8 pos = 0;
  do {
    if (tempmap[pos] && cand-- == 0) { break; }
  } while (++pos);
  return pos;
}

const u8 randmask[] = {
  0,0,
  1,
  3,3,
  7,7,7,7,
  15,15,15,15,15,15,15,15,
  31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,
  63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,
  63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,
  127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,
  127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,
  127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,
  127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
};

u8 randint(u8 mx) {
  if (mx == 0) { return 0; }
  u8 result;
  do {
    result = URAND() & randmask[mx];
  } while (result >= mx);
  return result;
}

#include <gb/gb.h>
#include <string.h>

#include "inventory.h"

#include "counter.h"
#include "gameplay.h"
#include "joy.h"
#include "mob.h"
#include "pickup.h"
#include "sound.h"
#include "sprite.h"
#include "targeting.h"

#pragma bank 1

#define INV_SELECT_FRAMES 8

#define INV_ANIM_FRAMES 25
#define INV_ROW_LEN 14
#define INV_HP_ADDR 0x9c22
#define INV_FLOOR_ADDR 0x9c27
#define INV_FLOOR_NUM_ADDR 0x9c2d
#define INV_KEYS_ADDR 0x9c25
#define INV_EQUIP_ADDR 0x9c63

#define INV_WIDTH 16
#define INV_HEIGHT 13
#define INV_DESC_ADDR 0x9d01
#define INV_SELECT_X_OFFSET 32
#define INV_SELECT_Y_OFFSET 40

#define INV_FLOOR_OFFSET 23      /* offset into inventory_map */
#define INV_BLANK_ROW_OFFSET 49  /* offset into inventory_map */
#define INV_BLIND_LEN 5          /* BLIND */
#define INV_FLOOR_3_SPACES_LEN 8 /* "FLOOR   " */

#define PICKUP_CHARGES 2

#define TILE_0 0xe5

extern const u8 inventory_map[];

extern const u8 inventory_up_y[];
extern const u8 inventory_down_y[];

extern const u8 pick_type_name_tile[];
extern const u8 pick_type_name_start[];
extern const u8 pick_type_name_len[];
extern const u8 pick_type_desc_tile[];
extern const u16 pick_type_desc_start[][NUM_INV_ROWS];
extern const u8 pick_type_desc_len[][NUM_INV_ROWS];

extern const u8 blind_map[];

u8 inv_anim_up;
u8 inv_anim_timer;
u8 inv_select;
u8 inv_select_timer;
u8 inv_select_frame;
u8 inv_msg_update;
PickupType inv_selected_pick;

u8 equip_type[MAX_EQUIPS];
u8 equip_charge[MAX_EQUIPS];

void inv_init(void) {
  inv_select_timer = INV_SELECT_FRAMES;
  inv_selected_pick = PICKUP_TYPE_NONE;
  inv_msg_update = 1;
  memset(equip_type, PICKUP_TYPE_NONE, sizeof(equip_type));
  set_win_tiles(0, 0, INV_WIDTH, INV_HEIGHT, inventory_map);
#ifdef CGB_SUPPORT
  if (_cpu == CGB_TYPE) {
    VBK_REG = 1;
    fill_win_rect(0, 0, INV_WIDTH, INV_HEIGHT, 3);
    set_vram_byte((u8 *)(INV_KEYS_ADDR - 1), 0); // make key yellow
    VBK_REG = 0;
  }
#endif
}

void inv_update(void) {
  if (!inv_anim_timer) {
    if (joy_action & (J_UP | J_DOWN)) {
      if (joy_action & J_UP) {
        inv_select += 3;
      } else if (joy_action & J_DOWN) {
        ++inv_select;
      }
      inv_select &= 3;
      inv_msg_update = 1;
      inv_selected_pick = equip_type[inv_select];
      sfx(SFX_SELECT);
    }
    if (newjoy & J_B) {
      inv_close();
      sfx(SFX_BACK);
    } else if (joy_action & J_A) {
      if (inv_selected_pick == PICKUP_TYPE_NONE) {
        sfx(SFX_OOPS);
      } else {
        inv_close();
        is_targeting = 1;
        sfx(SFX_OK);
      }
    } else if (joy == (J_LEFT | J_START)) {
      // Display level seed instead of the floor
      static const u8 hextile[] = {
          0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec,
          0xed, 0xee, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0,
      };

      set_vram_byte((u8 *)(INV_FLOOR_ADDR), hextile[(floor_seed >> 12) & 0xf]);
      set_vram_byte((u8 *)(INV_FLOOR_ADDR + 1),
                    hextile[(floor_seed >> 8) & 0xf]);
      set_vram_byte((u8 *)(INV_FLOOR_ADDR + 2),
                    hextile[(floor_seed >> 4) & 0xf]);
      set_vram_byte((u8 *)(INV_FLOOR_ADDR + 3), hextile[floor_seed & 0xf]);
      set_vram_byte((u8 *)(INV_FLOOR_ADDR + 4), TILE_0 + dogate);
      set_vram_byte((u8 *)(INV_FLOOR_ADDR + 5), 0);
      set_vram_byte((u8 *)(INV_FLOOR_ADDR + 6), 0);
      set_vram_byte((u8 *)(INV_FLOOR_ADDR + 7), 0);
    }
    joy_action = 0;
  }
}

void inv_animate(void) {
  if (inv_anim_timer) {
    --inv_anim_timer;
    WY_REG = inv_anim_up ? inventory_up_y[inv_anim_timer]
                         : inventory_down_y[inv_anim_timer];
  }

  if (inv_anim_timer || inv_anim_up) {
    if (--inv_select_timer == 0) {
      inv_select_timer = INV_SELECT_FRAMES;
      ++inv_select_frame;
    }
    *next_sprite++ = WY_REG + INV_SELECT_Y_OFFSET + (inv_select << 3);
    *next_sprite++ = INV_SELECT_X_OFFSET + pickbounce[inv_select_frame & 7];
    *next_sprite++ = 0;
    *next_sprite++ = 0;

    if (inv_msg_update) {
      inv_msg_update = 0;

      u16 addr = INV_DESC_ADDR;
      u8 pick = equip_type[inv_select];
      u8 i;
      for (i = 0; i < NUM_INV_ROWS; ++i) {
        u8 len = pick_type_desc_len[pick][i];
        if (len) {
          vram_copy(addr, pick_type_desc_tile + pick_type_desc_start[pick][i],
                    len);
        }

        // Fill the rest of the row with 0
        vmemset((u8 *)addr + len, 0, INV_ROW_LEN - len);
        addr += 32;
      }
    }
  }
}

void inv_display_blind(void) {
  vram_copy(INV_FLOOR_ADDR, blind_map, INV_BLIND_LEN);
  counter_thirty(&st_recover);
  counter_out(&st_recover, INV_FLOOR_NUM_ADDR);
}

void inv_display_floor(void) {
  vram_copy(INV_FLOOR_ADDR, inventory_map + INV_FLOOR_OFFSET,
            INV_FLOOR_3_SPACES_LEN);
  counter_out(&st_floor, INV_FLOOR_NUM_ADDR);
}

void inv_decrement_recover(void) {
  // Decrement recover counter
  counter_dec(&st_recover);
  // Blank out the right tile, in case of 10 -> 9
  set_vram_byte((u8 *)(INV_FLOOR_NUM_ADDR + 1), 0);
  counter_out(&st_recover, INV_FLOOR_NUM_ADDR);
}

void inv_update_hp(void) {
  set_vram_byte((u8 *)INV_HP_ADDR, TILE_0 + mob_hp[PLAYER_MOB]);
}

void inv_update_keys(void) {
  set_vram_byte((u8 *)INV_KEYS_ADDR, TILE_0 + num_keys);
}

void inv_open(void) {
  inv_anim_timer = INV_ANIM_FRAMES;
  inv_anim_up = 1;
}

void inv_close(void) {
  inv_anim_timer = INV_ANIM_FRAMES;
  inv_anim_up = 0;
}

u8 inv_acquire_pickup(PickupType ptype) {
  u8 i, len = pick_type_name_len[ptype];
  u16 equip_addr;

  // First check whether this equipment already exists, so we can
  // increase the number of charges
  equip_addr = INV_EQUIP_ADDR;
  for (i = 0; i < MAX_EQUIPS; ++i) {
    if (equip_type[i] == ptype) {
      // Increase charges
      if (equip_charge[i] < 9) {
        equip_charge[i] += PICKUP_CHARGES;
        if (equip_charge[i] > 9) {
          equip_charge[i] = 9;
        }
        set_vram_byte((u8 *)(equip_addr + len - 2), TILE_0 + equip_charge[i]);
      }
      return 1;
    }
    equip_addr += 32;
  }

  // Next check whether there is a new spot for this equipment
  equip_addr = INV_EQUIP_ADDR;
  for (i = 0; i < MAX_EQUIPS; ++i) {
    if (equip_type[i] == PICKUP_TYPE_NONE) {
      // Use this slot
      equip_type[i] = ptype;
      equip_charge[i] = PICKUP_CHARGES;
      vram_copy(equip_addr, pick_type_name_tile + pick_type_name_start[ptype],
                len);

      // Update the inventory message if the selection was set to this
      // equip slot
      if (inv_select == i) {
        inv_selected_pick = equip_type[inv_select];
        inv_msg_update = 1;
      }
      return 1;
    }
    equip_addr += 32;
  }

  return 0;
}

void inv_use_pickup(void) {
  u16 equip_addr = INV_EQUIP_ADDR - 2 + (inv_select << 5);
  if (--equip_charge[inv_select] == 0) {
    equip_type[inv_select] = PICKUP_TYPE_NONE;
    inv_selected_pick = PICKUP_TYPE_NONE;
    inv_msg_update = 1;

    // Clear equip display
    vram_copy(equip_addr, inventory_map + INV_BLANK_ROW_OFFSET, INV_ROW_LEN);
  } else {
    // Update charge count display
    set_vram_byte((u8 *)(equip_addr + pick_type_name_len[inv_selected_pick]),
                  TILE_0 + equip_charge[inv_select]);
  }
}

#include <gb/gb.h>
#include <string.h>

#include "sprite.h"

#include "float.h"
#include "msg.h"
#include "spr.h"
#include "targeting.h"

#pragma bank 1

u8 *next_sprite, *last_next_sprite;

void sprite_hide(void) {
  memset(shadow_OAM, 0, 160);
  float_hide();
  msg_hide();
  spr_hide();
}

void sprite_animate_begin(void) {
  // Start mob sprites after floats. Double buffer the shadow_OAM so we don't
  // get partial updates when writing to it.
  next_sprite = (u8 *)((_shadow_OAM_base << 8) ^ 0x100);
}

void sprite_animate_end(void) {
  // Hide the rest of the sprites
  while (next_sprite < last_next_sprite) {
    *next_sprite++ = 0;  // hide sprite
    next_sprite += 3;
  }
  last_next_sprite = next_sprite;

  // Flip the shadow_OAM buffer
  _shadow_OAM_base ^= 1;
}

void sprite_update(void) {
  msg_update();
  float_update();
  spr_update();
  targeting_update();
}


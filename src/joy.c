#include <gb/gb.h>

#include "joy.h"

#define JOY_REPEAT_FIRST_WAIT_FRAMES 30
#define JOY_REPEAT_NEXT_WAIT_FRAMES 4

#pragma bank 1

u8 joy, lastjoy, newjoy, repeatjoy;
u8 joy_repeat_count[8];
u8 joy_action; // The most recently pressed action button

void joy_init(void) {
  joy_action = 0;
}

void joy_update(void) {
  lastjoy = joy;
  joy = joypad();
  newjoy = (joy ^ lastjoy) & joy;
  repeatjoy = newjoy;

  if (joy) {
    u8 mask = 1, i = 0;
    while (mask) {
      if (newjoy & mask) {
        joy_repeat_count[i] = JOY_REPEAT_FIRST_WAIT_FRAMES;
      } else if (joy & mask) {
        if (--joy_repeat_count[i] == 0) {
          repeatjoy |= mask;
          joy_repeat_count[i] = JOY_REPEAT_NEXT_WAIT_FRAMES;
        }
      }

      if (repeatjoy & mask & (J_LEFT | J_RIGHT | J_UP | J_DOWN | J_A)) {
        joy_action = mask;
      }

      mask <<= 1;
      ++i;
    }
  }
}

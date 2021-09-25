#include "main.h"

void tim_interrupt(void);

void soundinit(void) {
  // set up sound registers
  NR52_REG = 0b10000000; // enable sound

  // set up timer for sound
  TAC_REG = 0b100;     // timer enabled, clock / 1024
  TMA_REG = 0xde;      // clock / 1024 / 34 = ~120Hz
  IE_REG |= TIM_IFLAG; // timer interrupt enabled
  add_TIM(tim_interrupt);
}

void tim_interrupt(void) {
}

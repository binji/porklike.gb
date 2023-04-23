#include "main.h"

void snd_init(void);
void snd_update(void);
void tim_interrupt(void);

void soundinit(void) {
  // set up sound registers
  NR52_REG = 0b10000000; // enable sound
  NR50_REG = 0b01110111; // both speakers full volume
  NR51_REG = 0b11111111; // output to both speakers

  snd_init();

  // set up timer for sound
  TAC_REG = 0b100;     // timer enabled, clock / 1024
#ifdef CGB_SUPPORT
  if (_cpu == CGB_TYPE) {
    TMA_REG = 0xbc;      // double speed clock / 1024 / 68 = ~120Hz
  } else
#endif
  {
    TMA_REG = 0xde;      // clock / 1024 / 34 = ~120Hz
  }
  IE_REG |= TIM_IFLAG; // timer interrupt enabled
  add_TIM(tim_interrupt);
}

void tim_interrupt(void) {
  snd_update();
}

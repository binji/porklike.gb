#ifndef GAMEOVER_H_
#define GAMEOVER_H_

#include "common.h"

typedef enum GameOverState {
  GAME_OVER_NONE,
  GAME_OVER_DEAD,
  GAME_OVER_WIN,
  GAME_OVER_WAIT,
} GameOverState;

void gameover_update(void);

extern GameOverState gameover_state;
extern u8 gameover_timer;

#endif // GAMEOVER_H_

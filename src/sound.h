#ifndef SOUND_H_
#define SOUND_H_

#define SFX_BLIND 0
#define SFX_TELEPORT 1
#define SFX_BACK 2
#define SFX_OK 3
#define SFX_STAIRS 4
#define SFX_SELECT 5
#define SFX_HIT_PLAYER 6
#define SFX_HIT_MOB 7
#define SFX_OPEN_OBJECT 8
#define SFX_OOPS 9
#define SFX_PICKUP 10
#define SFX_USE_EQUIP 11
#define SFX_PLAYER_STEP 12
#define SFX_MOB_PUSH 13
#define SFX_BOOM 14
#define SFX_REAPER 15
#define SFX_SPIN 16
#define SFX_SPEAR 17
#define SFX_DESTROY_WALL 18
#define SFX_BUMP_TILE 19
#define SFX_HEART 20
#define SFX_FAIL 21

void soundinit(void);
void music_main(void);
void music_win(void);
void music_dead(void);
void sfx(u8 id);

#endif // SOUND_H_

#ifndef PALETTE_H_
#define PALETTE_H_

#include <gb/gb.h>

// When a mob is flashing, it is either because it has a key, or was recently
// hit. They are meant to have different behavior; when a mob is hit it will
// stay white for MOB_FLASH_FRAMES. But when a mob has the key, it will
// periodically flash white and then back to normal.
//
// On DMG, we use OBP1 (i.e. `S_PALETTE` below) for both, and change the
// palette colors periodically. This means that sometimes a hit mob won't
// appear white for the full MOB_FLASH_FRAMES.
//
// On CGB, we can do better and use separate palettes for each of these.
#define MOB_FLASH_HIT_PAL (S_PALETTE | 3)
#define MOB_FLASH_KEY_PAL (S_PALETTE | 4)

void pal_init(void);
void pal_update(void);
void pal_fadeout(void);
void pal_fadein(void);

#endif // PALETTE_H_

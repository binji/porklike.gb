#ifndef JOY_H_
#define JOY_H_

#include "common.h"

void joy_init(void);
void joy_update(void);

extern u8 joy, newjoy;
extern u8 joy_action;

#endif // JOY_H_

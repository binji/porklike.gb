#ifndef COMMON_H_
#define COMMON_H_

#include <stdint.h>

typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef void (*vfp)(void);

#ifdef __SDCC
#define PRESERVES_REGS(...) __preserves_regs(__VA_ARGS__)
#else
#define PRESERVES_REGS(...)
#endif

#endif // COMMON_H_

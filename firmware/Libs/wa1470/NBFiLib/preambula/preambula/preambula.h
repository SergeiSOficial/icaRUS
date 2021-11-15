#ifndef _PREAMBULA_H_
#define _PREAMBULA_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define RAND_MULL	0x1234
#define RAND_ADD	0x10

#define MAX_ITER	100
#define MAX_COEFF	6

uint32_t preambula(uint32_t seed, uint32_t *iter, uint32_t *coeff);

#ifdef __cplusplus
}
#endif

#endif

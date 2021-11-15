#include "preambula.h"
#include <stdint.h>
#include <stdlib.h>

static uint32_t _alu(uint32_t data, uint32_t tmp)
{
	uint32_t test = data ^ tmp;
	int32_t count = 0;
	while (test)
		count += test & 0x01, test >>= 1;
	count = abs(count - (32 - count)) >> 1;
	
	return count;
}

static uint32_t _factor(uint32_t data)
{
	uint32_t i, count, tmp, max = 0;
	for (i = 0, tmp = data; i < 32; i++)
	{
		count = _alu(data, tmp);
		tmp <<= 1;
		if (count > max && i)
			max = count;
	}
	for (i = 0, tmp = data; i < 32; i++)
	{
		count = _alu(data, tmp);
		tmp >>= 1;
		if (count > max && i)
			max = count;
	}
	return max;
}

static uint32_t _random(uint32_t seed)
{
	static uint32_t _seed;
	if (!seed)
	{
		_seed = _seed * RAND_MULL + RAND_ADD;
		_seed = _seed << 7 | _seed >> 23;
	}
	else
		_seed = seed;

	return _seed;
}

uint32_t preambula(uint32_t seed, uint32_t *iter, uint32_t *coeff)
{
	uint32_t _rand, _coeff, _iter = 0;
	_random(seed);
	while (_iter < MAX_ITER)
	{
		_rand = _random(0);
		_coeff = _factor(_rand);
		_iter++;
		if (_coeff < MAX_COEFF)
			break;
	}
	if (iter)
		*iter = _iter;
	if (coeff)
		*coeff = _coeff;

	return _rand;
}

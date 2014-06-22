/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <stdio.h>
#include <math.h>

#include "prime.h"

int is_prime(int v)
{
	int sq;

	if (v <= 1)
		return 0;

	sq = (int)sqrt((double)v);

	for (int i = 2; i <= sq; i++) {
		if (v % i == 0)
			return 0;
	}

	return 1;
}

/*
 * fetch prime that less than v
 */
int get_min_prime(int v)
{
	for (; v > 2; v--) {
		if (is_prime(v))
			return v;
	}

	return 2;
}

/*
 * fetch prime that great than v
 */
int get_max_prime(int v)
{
	int max_int = (1 << 30) - 1;

	for (; v < max_int; v++) {
		if (is_prime(v))
			return v;
	}

	return max_int;
}

int get_best_hash(int h)
{
	int min, max;
	
	min = get_min_prime(h);
	max = get_max_prime(h);

	h = h - min > max - h ? max : min;

	return h <= 1 ? 2 : h;
}

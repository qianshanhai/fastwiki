/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */

#ifndef FW_NJI

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "minilzo.h"
#include "lzo1x.h"

#define _LZO_HEAD_LEN 10

typedef lzo_align_t __LZO_MMODEL lzo_mem_t[ ((LZO1X_1_MEM_COMPRESS) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ];

LZO1X::LZO1X()
{
	wrkmem = NULL;

	init();
}

LZO1X::~LZO1X()
{
	if (wrkmem)
		free(wrkmem);	
}

int LZO1X::init()
{
	if (lzo_init() != LZO_E_OK)
		return -1;

	wrkmem = (void *)malloc(sizeof(lzo_mem_t));

	return 0;
}

int LZO1X::compress(char *out, size_t out_size, const char *in, size_t in_size)
{
	lzo_uint ret;
	int err;

	memset(out, 0, _LZO_HEAD_LEN);

	if ((err = lzo1x_1_compress((unsigned char *)in, in_size,
					(unsigned char *)out + _LZO_HEAD_LEN, &ret,
					(lzo_align_t __LZO_MMODEL *)wrkmem)) != LZO_E_OK) {
		return -1;
	}
	sprintf(out, "%09d", (unsigned int)in_size);

	return ret + _LZO_HEAD_LEN;
}

int LZO1X::decompress(char *out, size_t out_size, const char *in, size_t in_size)
{
	lzo_uint ret = out_size;
	int err;

	if ((err = lzo1x_decompress((unsigned char *)in + _LZO_HEAD_LEN,
					in_size - _LZO_HEAD_LEN, (unsigned char *)out,
					&ret, NULL)) != LZO_E_OK) {
		return -1;
	}
	
	return ret;
}

int LZO1X::data_len(const char *p)
{
	char buf[_LZO_HEAD_LEN+1];

	memset(buf, 0, sizeof(buf));

	memcpy(buf, p, _LZO_HEAD_LEN);

	return atoi(buf);
}

#endif

/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */

#include <zlib.h>
#include <bzlib.h>

int gunzip(char *out, int out_len, const char *in, int in_len);
int gzip(char *out, int out_len, const char *in, int in_len);

inline int gunzip(char *out, int out_len, const char *in, int in_len)
{
	int err;
	unsigned long ret = (unsigned long)out_len;

	return ((err = uncompress((Bytef *)out, (unsigned long *)&ret,
					(Bytef *)in, (unsigned long)in_len)) != Z_OK) ? -1 : (int)ret;
}

inline int gzip(char *out, int out_len, const char *in, int in_len)
{
	int err;
	unsigned long ret = (unsigned long)out_len;

	return ((err = compress((Bytef *)out, (unsigned long *)&ret,
					(Bytef *)in, (unsigned long)in_len)) != Z_OK) ? -1 : (int)ret;
}

#if 1

inline int bzip2(char *out, int out_len, const char *in, int in_len)
{
	unsigned int tmp = out_len;

	if (BZ2_bzBuffToBuffCompress(out, &tmp, (char *)in, (unsigned int)in_len, 9, 0, 0) == BZ_OK)
		return tmp;

	return -1;
}

inline int bunzip2(char *out, int out_len, const char *in, int in_len)
{
	unsigned int tmp = out_len;

	if (BZ2_bzBuffToBuffDecompress(out, &tmp, (char *)in, (unsigned int)in_len, 0, 0) == BZ_OK)
		return tmp;

	return -1;
}
#endif

#include "lz4.h"
#include "lz4hc.h"

inline int lz4_compress(char *out, int out_len, const char *in, int in_len)
{
	int n;

	return (n = LZ4_compressHC(in, out, in_len, out_len)) == 0 ? -1 : n;
}

inline int lz4_decompress(char *out, int out_len, const char *in, int in_len)
{
	int n;
	
	return (n = LZ4_decompress_safe(in, out, in_len, out_len)) == 0 ? - 1: n;
}

#ifndef FW_NJI

#include "lzo1x.h"

static LZO1X *m_lzo1x = NULL;

/*
 * lzo1x_compress() and lzo1x_decompress() is not thread safe.
 */
inline int lzo1x_compress(char *out, int out_len, const char *in, int in_len)
{
	if (m_lzo1x == NULL)
		m_lzo1x = new LZO1X();

	return m_lzo1x->compress(out, out_len, in, in_len);
}

inline int lzo1x_decompress(char *out, int out_len, const char *in, int in_len)
{
	if (m_lzo1x == NULL)
		m_lzo1x = new LZO1X();

	return m_lzo1x->decompress(out, out_len, in, in_len);
}

#endif

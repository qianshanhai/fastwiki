/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wiki_common.h"
#include "q_util.h"

#include "gzip_compress.h"

int wiki_pthread_total()
{
	char *p = getenv("WIKI_PTHREAD");

	if (p == NULL || atoi(p) <= 0)
		return q_get_cpu_total();

	return atoi(p);
}

int wiki_is_dont_ask()
{
	char *env = getenv("DONT_ASK");

	if (env != NULL && atoi(env) == 1)
		return 1;

	return 0;
}

int wiki_is_mutil_output()
{
	char *env = getenv("FW_MUTIL_INDEX");

	if (env != NULL && atoi(env) == 1)
		return 1;

	return 0;
}

int wiki_is_complete()
{
	char *env = getenv("WIKI_COMPLETE");

	if (env != NULL && atoi(env) == 1)
		return 1;

	return 0;
}

static unsigned int __wiki_split_size = 1800*1024*1024;

void set_wiki_split_size(unsigned int m)
{
	__wiki_split_size = m * 1024 * 1024;
}

unsigned int wiki_split_size()
{
	char *env = getenv("WIKI_SPLIT_SIZE");

	if (env != NULL)
		return atoi(env) * 1024 * 1024;

	return __wiki_split_size;
}

char *format_image_name(char *buf, int max_len)
{
	char tmp[1024];
	int pos = 0, len;
	char *save = buf;

	if (strncasecmp(buf, "File:", 5) == 0)
		buf += 5;
	
	len = strlen(buf);

	for (int i = 0; i < len; i++) {
		if (buf[i] == '&') {
			if (strncasecmp(buf + i, "amp;", 4) == 0) {
				tmp[pos++] = '&';
				i += 4;
				continue;
			}
			if (strncasecmp(buf + i, "quot;", 5) == 0) {
				tmp[pos++] = '"';
				i += 5;
				continue;
			}
		}
		if (buf[i] == ':') {
			tmp[pos++] = '.';
			continue;
		}

		if (buf[i] >= 'A' && buf[i] <= 'Z') {
			tmp[pos++] = buf[i] + 'a' - 'A';
			continue;
		}

		if (buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\n' || buf[i] == '-' || buf[i] == '_') {
			continue;
		}
		tmp[pos++] = buf[i];
	}
	tmp[pos] = 0;

	strncpy(save, tmp, max_len - 1);
	save[max_len - 1] = 0;

	return buf;
}

unsigned char hex(char ch)
{
	unsigned char x = 0;

	if (ch >= 'a' && ch <= 'f')
		x = ch - 'a' + 10;
	else if (ch >= 'A' && ch <= 'F')
		x = ch - 'A' + 10;
	else if (ch >= '0' && ch <= '9')
		x = ch - '0';

	return x;
}

unsigned char hex2ch(const char *buf)
{
	unsigned char x, y;

	x = hex(buf[0]);
	y = hex(buf[1]);
	x = x * 16 + y;

	return (unsigned char)x;
}

#define TOLOWER(c) (((c) >= 'a' && (c) <= 'z') ? ((c) + 'A' - 'a') : (c))
#define TOHEX(c) (((c) >= '0' && (c) <= '9') ? ((c) - '0'): (TOLOWER(c) - 'A' + 10))

int ch2hex(const char *s, unsigned char *buf)
{
	int pos = 0;

	for (; *s; s++) {
		if (*s == '%') {
			if (s[1] != 0 && s[2] != 0) {
				buf[pos++] = TOHEX(s[1]) * 16 + TOHEX(s[2]);
				s += 2;
				continue;
			}
		}
		buf[pos++] = *s;
	}

	buf[pos] = 0;

	return pos;
}

char *url_convert(char *buf)
{
	char tmp[1024];
	int max_len = sizeof(tmp) - 1;
	int len = 0;

	for (int i = 0; i < max_len && buf[i]; i++) {
		if (buf[i] == '+') {
			tmp[len++] = ' ';
		} else if (buf[i] == '%') {
			tmp[len++] = hex2ch(buf + i + 1);
			i += 2;
		} else
			tmp[len++] = buf[i];
	}
	tmp[len] = 0;

	strcpy(buf, tmp);

	return buf;
}

static char texvc_file[256];

char *get_texvc_file()
{
	return texvc_file;
}

int init_texvc_file()
{
	char file[1024];
	char *path = getenv("PATH");
	split_t sp;

	memset(texvc_file, 0, sizeof(texvc_file));

	if (path == NULL)
		return -1;

	split(':', path, sp);

	for_each_split(sp, item) {
		sprintf(file, "%s/texvc", item);		
		if (dashf(file)) {
			strcpy(texvc_file, file);
			return 0;
		}
	}

	return -1;
}

compress_func_t get_compress_func(int *flag, const char *str)
{
	if (strcasecmp(str, "text") == 0 || strcasecmp(str, "txt") == 0) {
		*flag = FM_FLAG_TEXT;
		return NULL;
	} else if (strcasecmp(str, "bzip2") == 0) {
		*flag = FM_FLAG_BZIP2;
		return bzip2;
	} else if (strcasecmp(str, "gzip") == 0) {
		*flag = FM_FLAG_GZIP;
		return gzip;
	} else if (strcasecmp(str, "lz4") == 0) {
		*flag = FM_FLAG_LZ4;
		return lz4_compress;
	} else if (strcasecmp(str, "lzo1x") == 0) {
#ifndef FW_NJI
		*flag = FM_FLAG_LZO1X;
		return lzo1x_compress;
#endif
	}

	*flag = -1;

	return NULL;
}

compress_func_t get_decompress_func(int z_flag)
{
	compress_func_t ret = NULL;

	switch (z_flag) {
		case FM_FLAG_BZIP2:
			ret = bunzip2;
			break;
		case FM_FLAG_GZIP:
			ret = gunzip;
			break;
		case FM_FLAG_LZ4:
			ret = lz4_decompress;
			break;
#ifndef FW_NJI
		case FM_FLAG_LZO1X:
			ret = lzo1x_decompress;
			break;
#endif
		default:
			break;
	}

	return ret;
}

/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "q_util.h"
#include "gzip_compress.h"

#include "wiki_common.h"
#include "wiki_full_index.h"

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
#ifndef FW_NJI
		*flag = FM_FLAG_BZIP2;
		return bzip2;
#endif
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
		case FM_FLAG_GZIP:
			ret = gunzip;
			break;
		case FM_FLAG_LZ4:
			ret = lz4_decompress;
			break;
#ifndef FW_NJI
		case FM_FLAG_BZIP2:
			ret = bunzip2;
			break;
		case FM_FLAG_LZO1X:
			ret = lzo1x_decompress;
			break;
#endif
		default:
			break;
	}

	return ret;
}

#define _MAX_WORD_TOTAL 128

typedef struct {
	char *val;
	int val_len;
	int total;
	int pos;
} word_t;

#define _to_lower(x) (((x) >= 'A' && (x) <= 'Z') ? (x) + 'a' - 'A' : (x))
#define _is_a2z(x) (((x) >= 'a' && (x) <= 'z') || ((x) >= '0' && (x) <= '9'))
#define _is_mutil_byte(x) (((unsigned char)(x)) & 0x80)

#define _RED_FONT_START "<font color=red>"
#define _RED_FONT_END "</font>"

#define _copy_to_tmp(buf, size) \
	do { \
		if (size > 0) { \
			memcpy(tmp + tmp_pos, buf, size); \
			tmp_pos += size; \
		} \
	} while (0)

int _cmp_word_pos(const void *a, const void *b)
{
	word_t *pa = (word_t *)a;
	word_t *pb = (word_t *)b;

	return pa->pos - pb->pos;
}


int check_word(const word_t *word, int total, const char *T)
{
	for (int i = 0; i < total; i++) {
		if (strcmp(word[i].val, T) == 0)
			return 1;
	}

	return 0;
}

int fetch_word_from_key(word_t *word, int max_total, split_t &sp, const char *key)
{
	int total = 0;

	split(' ', key, sp);

	for_each_split(sp, T) {
		if (total >= max_total)
			break;
		if (T[0] && !check_word(word, total, T)) {
			word_t *t = &word[total++];
			memset(t, 0, sizeof(word_t));
			t->val = T;
			t->val_len = strlen(T);
			t->total = 0;
		}
	}

	return total;
}

int convert_page_simple(char *tmp, char *buf, int len, const char *key)
{
	int tmp_pos = 0;
	int word_total = 0;
	split_t sp;
	word_t word[_MAX_WORD_TOTAL];

	word_total = fetch_word_from_key(word, _MAX_WORD_TOTAL, sp, key);

	for (int i = 0; i < len; i++) {
		if (buf[i] == '<') {
			for (; i < len && buf[i] != '>'; i++) {
				tmp[tmp_pos++] = buf[i];
			}
			tmp[tmp_pos++] = buf[i];
			continue;
		}

		for (int w = 0; w < word_total; w++) {
			word_t *t = &word[w];
			if (t->val[0] == _to_lower(buf[i]) && strncasecmp(t->val, buf + i, t->val_len) == 0) {
				if (_is_mutil_byte(buf[i]) || (!_is_a2z(buf[i + t->val_len])
							&& (i == 0 || (i > 0 && !_is_a2z(buf[i - 1]))))) {
					_copy_to_tmp(_RED_FONT_START, sizeof(_RED_FONT_START) - 1);
					_copy_to_tmp(t->val, t->val_len);
					_copy_to_tmp(_RED_FONT_END, sizeof(_RED_FONT_END) - 1);
					i += t->val_len - 1;
					goto out;
				}
			}
		}
		tmp[tmp_pos++] = buf[i];
out:
		;
	}

	tmp[tmp_pos] = 0;

	return tmp_pos;
}

int delete_html_tag(char *out, char *buf, int len)
{
	int pos = 0;

	for (int i = 0; i < len; i++) {
		if (buf[i] == '<') {
			for (; i < len && buf[i] != '>'; i++);
			continue;
		}
		out[pos++] = buf[i];
	}

	out[pos] = 0;

	return pos;
}

int convert_page_complex(char *tmp, char *buf, int len, const char *key)
{
	int word_total = 0;
	split_t sp;
	word_t word[_MAX_WORD_TOTAL];
	int find_total = 0;

	word_total = fetch_word_from_key(word, _MAX_WORD_TOTAL, sp, key);

	len = delete_html_tag(tmp, buf, len);
	memcpy(buf, tmp, len);

	for (int i = 0; i < word_total; i++) {
		word_t *t = &word[i];
		for (int j = 0; j < len; j++) {
			if (t->val[0] == _to_lower(buf[j]) && strncasecmp(buf + j, t->val, t->val_len) == 0) {
				if (_is_mutil_byte(buf[j]) || (!_is_a2z(buf[j + t->val_len])
							&& (j == 0 || (j > 0 && !_is_a2z(buf[j - 1]))))) {
					t->total = 1;
					t->pos = j;
					find_total++;
					break;
				}
			}
		}
	}

	int tmp_pos = 0;
	int find_pos = 0;

#ifdef DEBUG 
	for (int i = 0; i < word_total; i++) {
		word_t *t = &word[i];
		printf("word:%s, pos: %d\n", t->val, t->pos);
	}
#endif

	qsort(word, word_total, sizeof(word_t), _cmp_word_pos);


	int mb_count = 0;

	for (int i = word[0].pos - 1; i >= 0; i--) {
		if (word[0].pos - i > 80) {
			for (; i >= 0; i--) {
				if (_is_mutil_byte(buf[i])) {
					mb_count++;
					if (mb_count % _WFI_CHECK_WORD_LEN == 0)
						break;
				} else if (!_is_a2z(buf[i])) {
					break;
				}
			}
			find_pos = i;
			break;
		}

		if (_is_mutil_byte(buf[i]))
			mb_count++;

		if (buf[i] == '\n') {
			find_pos = i;
			break;
		}
	}

	_copy_to_tmp(buf + find_pos, word[0].pos - find_pos);

	for (int i = 0; i < word_total; i++) {
		word_t *t = &word[i];
		word_t *last = &word[i - 1];
		if (i > 0) {
			int found = 0;
			mb_count = 0;
			for (int j = last->pos + last->val_len; j < t->pos; j++) {
				if (j - (last->pos + last->val_len) > 80) {
					for (; j < t->pos; j++) {
						tmp[tmp_pos++] = buf[j];
						if (_is_mutil_byte(buf[j])) {
							mb_count++;
							if (mb_count % _WFI_CHECK_WORD_LEN == 0)
								break;
						} else if (!_is_a2z(buf[j])) {
							break;
						}
					}
					tmp[tmp_pos++] = '#';
					tmp[tmp_pos++] = ' ';
					found = 1;
					break;
				}
				if (buf[j] == '\n') {
					found = 1;
					break;
				}
				if (_is_mutil_byte(buf[j]))
					mb_count++;
				tmp[tmp_pos++] = buf[j];
			}

			if (found == 1) {
				find_pos = 0;
				mb_count = 0;
				for (int j = t->pos - 1; j >= 0; j--) {
					if (t->pos - j > 80) {
						for (; j >= 0; j--) {
							if (_is_mutil_byte(buf[j])) {
								mb_count++;
								if (mb_count % _WFI_CHECK_WORD_LEN == 0)
									break;
							} else if (!_is_a2z(buf[j]))
								break;
						}
						find_pos = j;
						break;
					}
					if (buf[j] == '\n') {
						find_pos = j + 1;
						tmp[tmp_pos++] = '#';
						tmp[tmp_pos++] = ' ';
						break;
					}
					if (_is_mutil_byte(buf[j]))
						mb_count++;
				}
				for (int j = find_pos; j < t->pos; j++) {
					tmp[tmp_pos++] = buf[j];
				}
			}
		}
		_copy_to_tmp(_RED_FONT_START, sizeof(_RED_FONT_START) - 1);
		_copy_to_tmp(t->val, t->val_len);
		_copy_to_tmp(_RED_FONT_END, sizeof(_RED_FONT_END) - 1);
	}

	word_t *next = &word[word_total - 1];

	mb_count = 0;
	for (int i = next->pos + next->val_len; i < len; i++) {
		if (i - (next->pos + next->val_len) > 80) {
			for (; i < len; i++) {
				tmp[tmp_pos++] = buf[i];
				if (_is_mutil_byte(buf[i])) {
					mb_count++;
					if (mb_count % _WFI_CHECK_WORD_LEN == 0)
						break;
				} else if (!_is_a2z(buf[i]))
					break;
			}
			break;
		}
		tmp[tmp_pos++] = buf[i];
		if (buf[i] == '\n')
			break;
		if (_is_mutil_byte(buf[i]))
			mb_count++;
	}

	tmp[tmp_pos] = 0;
	
	return tmp_pos;
}

int wiki_convert_key(WikiIndex *index, const char *start, const char *key, int key_len, char *buf)
{
	int len, len2, found;
	char tmp[KEY_SPLIT_LEN][MAX_KEY_LEN];
	char tmp2[KEY_SPLIT_LEN][MAX_KEY_LEN];
	int size = 0;
	char space[4];

	space[1] = 0;

	memset(tmp, 0, sizeof(tmp));
	memset(tmp2, 0, sizeof(tmp2));

	index->wi_split_key(key, key_len, tmp, &len);
	index->wi_split_key(start, strlen(start), tmp2, &len2);

	for (int i = 0; i < len; i++) {
		found = 0;
		for (int j = 0; j < len2; j++) {
			if (my_strcmp(tmp[i], tmp2[j], 0) == 0) {
				found = 1;
				break;
			}
		}
		space[0] = (tmp[i][0] & 0x80) ? 0 : ' ';

		if (found == 0) {
			size += sprintf(buf + size, "%s%s", tmp[i], space);
		} else {
			size += sprintf(buf + size, "<font color=red>%s</font>%s", tmp[i], space);
		}
	}
	buf[size] = 0;

	return 0;
}


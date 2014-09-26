/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __WIKI_ENV_COMMON_H
#define __WIKI_ENV_COMMON_H

#include "wiki_index.h"

enum {
	FM_FLAG_TEXT = 0x10,
	FM_FLAG_BZIP2 = 0x20,
	FM_FLAG_GZIP = 0x40,
	FM_FLAG_LZ4 = 0x80,
	FM_FLAG_LZO1X = 0x100
};

#define MAX_FD_TOTAL 64
typedef char fw_files_t[MAX_FD_TOTAL][128]; 

typedef int (*compress_func_t)(char *out, int out_len, const char *in, int in_len);

#ifdef __cplusplus
extern "C" {
#endif

int wiki_pthread_total();

int wiki_is_dont_ask();
int wiki_is_mutil_output();
int wiki_is_complete();

void set_wiki_split_size(unsigned int m);
unsigned int wiki_split_size();

char *format_image_name(char *buf, int max_len);

unsigned char hex(char ch);
unsigned char hex2ch(const char *buf);

int ch2hex(const char *s, unsigned char *buf);

char *url_convert(char *buf);

char *get_texvc_file();
int init_texvc_file();

compress_func_t get_compress_func(int *flag, const char *str);
compress_func_t get_decompress_func(int z_flag);

int convert_page_simple(char *tmp, char *buf, int len, const char *key);
int convert_page_complex(char *tmp, char *buf, int len, const char *key);

int wiki_convert_key(WikiIndex *index, const char *start, const char *key, int key_len, char *buf);

#ifdef __cplusplus
};
#endif

#endif

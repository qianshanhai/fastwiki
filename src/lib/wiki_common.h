/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __WIKI_ENV_COMMON_H
#define __WIKI_ENV_COMMON_H

enum {
	FM_FLAG_TEXT = 0x10,
	FM_FLAG_BZIP2 = 0x20,
	FM_FLAG_GZIP = 0x40,
	FM_FLAG_LZ4 = 0x80
};

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FD_TOTAL 32

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

#ifdef __cplusplus
};
#endif

#endif

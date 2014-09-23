/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __WIKI_BOOKMARK_H
#define __WIKI_BOOKMARK_H

#include "s_hash.h"

typedef struct {
	unsigned int crc32;
	unsigned int r_crc32;
	unsigned int len;
} bookmark_key_t;

typedef struct {
	int pos;
	int height;
	int screen_height;
	unsigned int read_total;
	char reserve[4];
} bookmark_value_t;

class WikiBookmark {

	private:
		int m_init_flag;
		SHash *m_hash;

	public:
		WikiBookmark();
		~WikiBookmark();

	public:
		int wbm_init(const char *dir);
		int wbm_add_bookmark(const char *title, int len, int pos, int height, int screen_height);
		int wbm_find_bookmark(const char *title, int len, bookmark_value_t *v);

		int wbm_copy_key(bookmark_key_t *k, const char *title, int len);
};

#endif

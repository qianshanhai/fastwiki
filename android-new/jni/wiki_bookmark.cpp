/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "crc32sum.h"
#include "wiki_bookmark.h"

WikiBookmark::WikiBookmark()
{
}

WikiBookmark::~WikiBookmark()
{
}

int WikiBookmark::wbm_init(const char *dir)
{
	char file[128];

	m_init_flag = 0;

	m_hash = new SHash();
	m_hash->sh_set_hash_magic(10000);

	sprintf(file, "%s/fastwiki.bookmark", dir);

	if (m_hash->sh_init(file, sizeof(bookmark_key_t), sizeof(bookmark_value_t), 2000, 1) == -1)
		return -1;

	m_init_flag = 1;

	return 0;
}

int WikiBookmark::wbm_copy_key(bookmark_key_t *k, const char *title, int len)
{
	memset(k, 0, sizeof(bookmark_key_t));

	crc32sum(title, len, &k->crc32, &k->r_crc32);
	k->len = len;

	return 0;
}

int WikiBookmark::wbm_add_bookmark(const char *title, int len, int pos, int height, int screen_height)
{
	bookmark_key_t key;
	bookmark_value_t value, *f;

	wbm_copy_key(&key, title, len);

	if (m_hash->sh_find(&key, (void **)&f) == _SHASH_FOUND) {
		if (f->pos > 0) {
			f->pos = pos;
			f->height = height;
			f->screen_height = screen_height;
		}

		return 0;
	}

	memset(&value, 0, sizeof(value));
	value.pos = pos;
	value.height = height;
	value.screen_height = screen_height;

	m_hash->sh_add(&key, &value);

	return 0;
}

int WikiBookmark::wbm_find_bookmark(const char *title, int len, bookmark_value_t *v)
{
	bookmark_key_t key;
	bookmark_value_t *f;

	wbm_copy_key(&key, title, len);
	memset(v, 0, sizeof(bookmark_value_t));

	if (m_hash->sh_find(&key, (void **)&f) == _SHASH_FOUND) {
		memcpy(v, f, sizeof(bookmark_value_t));
		if (v->pos < 0 || f->height <= 0)
			v->pos = 0;
	}

	return 0;
}

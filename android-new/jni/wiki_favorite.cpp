/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "prime.h"
#include "wiki_favorite.h"
#include "wiki_config.h"

WikiFavorite::WikiFavorite()
{
	m_hash = NULL;
}

WikiFavorite::~WikiFavorite()
{
	if (m_hash)
		delete m_hash;
}

int WikiFavorite::wf_init()
{
	m_hash = new SHash();
	m_hash->sh_set_hash_magic(get_best_hash(1024));

	if (m_hash->sh_init(BASE_DIR "/fastwiki.favorite", sizeof(struct fav_key),
					sizeof(struct fav_value), 200, 1) == -1) {
		return -1;
	}

	return 0;
}

int WikiFavorite::wf_set_key(struct fav_key *key, const char *title, const char *lang)
{
	memset(key, 0, sizeof(*key));

	strncpy(key->title, title, sizeof(key->title) - 1);
	strncpy(key->lang, lang, sizeof(key->lang) - 1);

	return 0;
}

int WikiFavorite::wf_add(const char *title, const char *lang, int size)
{
	struct fav_key key;
	struct fav_value value;

	wf_set_key(&key, title, lang);

	memset(&value, 0, sizeof(value));
	value.last_time = (int)time(NULL);
	value.size = size;

	m_hash->sh_replace(&key, &value);

	return 0;
}

int WikiFavorite::wf_find(const char *title, const char *lang)
{
	struct fav_key key;
	struct fav_value *f;

	wf_set_key(&key, title, lang);

	if (m_hash->sh_find(&key, (void **)&f) == _SHASH_FOUND)
		return 1;

	return 0;
}

int WikiFavorite::wf_delete(const char *title, const char *lang)
{
	struct fav_key key;

	wf_set_key(&key, title, lang);
	m_hash->sh_delete(&key);

	return 0;
}

int WikiFavorite::wf_delete_all()
{
	m_hash->sh_clean();

	return 0;
}

int WikiFavorite::wf_begin()
{
	m_hash->sh_reset();

	return 0;
}

int WikiFavorite::wf_next(struct fav_key **p, struct fav_value *ret_value)
{
	struct fav_key k;
	struct fav_value *value;

	if (m_hash->sh_read_next(&k, (void **)&value) == _SHASH_FOUND) {
		m_hash->sh_get_key_by_value(value, (void **)p);
		memcpy(ret_value, value, sizeof(struct fav_value));
		return 1;
	}

	return 0;
}

int WikiFavorite::wf_random(struct fav_key *key, struct fav_value *value)
{
	return m_hash->sh_random(key, value);
}

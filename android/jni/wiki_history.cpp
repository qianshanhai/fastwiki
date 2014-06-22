/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "wiki_history.h"

WikiHistory::WikiHistory()
{
	m_hash = NULL;
	m_curr = m_cache_head = NULL;

	m_cache_total = 0;
}

WikiHistory::~WikiHistory()
{
	wh_exit();

	if (m_hash)
		delete m_hash;
}

#define BASE_DIR "/sdcard/hace/fastwiki"

int WikiHistory::wh_init()
{
	m_hash = new SHash();
	m_hash->sh_init(BASE_DIR "/fastwiki.history",
				sizeof(history_key_t), sizeof(history_value_t), 1000, 1);

	return 0;
}

int WikiHistory::wh_exit()
{
	return 0;
}

int WikiHistory::wh_clean_cache()
{
	cache_t *t, *p = m_cache_head;

	while (p) {
		t = p->next;
		free(p);
		p = t;
	}

	m_cache_total = 0;
	m_curr = m_cache_head = NULL;
	
	return 0;
}

int WikiHistory::wh_delete_cache(cache_t *p)
{
	cache_t *prev = p->prev;
	cache_t *next = p->next;

	if (next)
		next->prev = prev;

	if (prev)
		prev->next = next;

	if (p == m_curr) {
		m_curr = m_cache_head = NULL;
	}

	free(p);

	return 0;
}

cache_t *WikiHistory::wh_new_cache(int status, const char *title, const char *lang,
			int size, const sort_idx_t *idx)
{
	cache_t *p = (cache_t *)malloc(sizeof(cache_t));

	memset(p, 0, sizeof(*p));

	p->next = p->prev = NULL;
	p->flag = status;

	strncpy(p->key.title, title, sizeof(p->key.title) - 1);

	if (status == STATUS_CONTENT) {
		strncpy(p->key.lang, lang, sizeof(p->key.lang) - 1);
		p->value.size = size;
		p->value.time = (int)time(NULL);
		memcpy(&p->idx, idx, sizeof(sort_idx_t));
		m_cache_total++;
		wh_add_history(&p->key, &p->value);
	}

	return p;
}

/*
        p
 curr       curr->next
  
*/
int WikiHistory::wh_add_cache(int status, const char *title, const char *lang,
			int size, const sort_idx_t *idx)
{
	cache_t *p = wh_new_cache(status, title, lang, size, idx);

	if (m_cache_head == NULL) {
		m_cache_head = p;
	} else {
		p->next = m_curr->next;
		p->prev = m_curr;

		if (m_curr->next) { 
			m_curr->next->prev = p;
		}

		m_curr->next = p;
	}
	m_curr = p;

	return 0;
}

const cache_t *WikiHistory::wh_curr_cache()
{
	return m_curr;
}

const cache_t *WikiHistory::wh_back_cache()
{
	if (m_curr == m_cache_head)
		return NULL;

	m_curr = m_curr->prev;

	return m_curr;
}

const cache_t *WikiHistory::wh_forward_cache()
{
	if (m_curr->next == NULL)
		return NULL;

	m_curr = m_curr->next;

	return m_curr;
}

int WikiHistory::wh_curr_cache_begin()
{
	m_begin_idx = m_cache_head;

	return 0;
}

int WikiHistory::wh_curr_cache_next(cache_t **p)
{
	cache_t *save;

	while (m_begin_idx != NULL) {
		save = m_begin_idx;
		m_begin_idx = m_begin_idx->next;
		if (save->flag == STATUS_CONTENT) {
			*p = save;
			return 1;
		}
	}

	return 0;
}

int WikiHistory::wh_set_key(history_key_t *key, const char *title, const char *lang)
{
	memset(key, 0, sizeof(*key));

	strncpy(key->title, title, sizeof(key->title) - 1);
	strncpy(key->lang, lang, sizeof(key->lang) - 1);

	return 0;
}

int WikiHistory::wh_add_history(const history_key_t *key, const history_value_t *value)
{
	history_value_t *f;

	if (m_hash->sh_find(key, (void **)&f) == _SHASH_FOUND) {
		f->size = value->size;
		f->time = value->time;
		return 0;
	}

	m_hash->sh_add(key, value);

	return 0;
}

int WikiHistory::wh_delete_history(const char *title, const char *lang)
{
	history_key_t key;
	
	wh_set_key(&key, title, lang);
	m_hash->sh_delete(&key);

	return 0;
}

int WikiHistory::wh_clean_history()
{
	m_hash->sh_clean();

	return 0;
}

int WikiHistory::wh_history_begin()
{
	m_hash->sh_reset();

	return 0;
}

int WikiHistory::wh_history_next(history_key_t **p, history_value_t *ret_value)
{
	history_key_t k;
	history_value_t *value;

	if (m_hash->sh_read_next(&k, (void **)&value) == _SHASH_FOUND) {
		m_hash->sh_get_key_by_value(value, (void **)p);
		memcpy(ret_value, value, sizeof(history_value_t));
		return 1;
	}

	return 0;
}

int WikiHistory::wh_total()
{
	return m_hash->sh_hash_total();
}

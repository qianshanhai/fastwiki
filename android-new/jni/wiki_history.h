/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __WIKI_HISTORY_H
#define __WIKI_HISTORY_H

#include "s_hash.h"
#include "wiki_index.h"

/* status */
#define STATUS_MATCH_KEY 0x1
#define STATUS_CONTENT 0x2
#define STATUS_NOT_FOUND 0x4

typedef struct history_key {
	char title[120];
	char lang[32];
} history_key_t;

typedef struct history_value {
	int size;
	int time;
	char unused[8];
} history_value_t;

typedef struct cache_t {
	history_key_t key;
	history_value_t value;
	sort_idx_t idx;
	int flag; /* STATUS_xx */
	cache_t *next;
	cache_t *prev;
} cache_t;

typedef struct history_head {
	history_key_t last_key;
	history_value_t last_value;
} history_head_t;

#define get_cache_key(p) (&((p)->key))
#define get_cache_value(p) (&((p)->value))

#define WH_CACHE_MAX_TOTAL 1000

class WikiHistory {
	private:
		SHash *m_hash;
		history_head_t *m_head;

		cache_t *m_cache_head;
		cache_t *m_curr;
		cache_t *m_begin_idx;

		int m_cache_total;

	public:
		WikiHistory();
		~WikiHistory();

		int wh_init();
		int wh_exit();

		int wh_delete_cache(cache_t *p);
		cache_t *wh_new_cache(int status, const char *title, const char *lang,
						int size, const sort_idx_t *idx);
		int wh_add_cache(int status, const char *title, const char *lang,
					int size, const sort_idx_t *idx);

		const cache_t *wh_curr_cache();
		const cache_t *wh_back_cache();
		const cache_t *wh_forward_cache();

		int wh_curr_cache_begin();
		int wh_curr_cache_next(cache_t **p);
		
		int wh_clean_cache();

		int wh_set_key(history_key_t *key, const char *title, const char *lang);

		int wh_add_history(const history_key_t *key, const history_value_t *value);
		int wh_delete_history(const char *title, const char *lang);
		int wh_clean_history();

		int wh_history_begin();
		int wh_history_next(history_key_t **p, history_value_t *ret_value);

		int wh_set_last_record(const history_key_t *key, const history_value_t *value);
		int wh_get_last_record(history_key_t *last_key, history_value_t *last_value);

		int wh_total();
		int wh_random(history_key_t *key, history_value_t *value);
};

#endif

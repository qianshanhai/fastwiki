/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __WIKI_FAVORITE_H
#define __WIKI_FAVORITE_H

#include "s_hash.h"

struct fav_key {
	char title[120];
	char lang[32];
};

struct fav_value {
	int last_time;
	int size;
	float rate;
	char reserve[8];
};

class WikiFavorite {
	private:
		SHash *m_hash;

	public:
		WikiFavorite();
		~WikiFavorite();

		int wf_init();
		int wf_set_key(struct fav_key *key, const char *title, const char *lang);
		int wf_add(const char *title, const char *lang, int size);
		int wf_find(const char *title, const char *lang);
		int wf_delete(const char *title, const char *lang);
		int wf_delete_all();
		int wf_begin();
		int wf_next(struct fav_key **p, struct fav_value *ret_value);
};

#endif

/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __WIKI_MANAGE_H
#define __WIKI_MANAGE_H

#include <pthread.h>

#include "wiki_config.h"
#include "wiki_favorite.h"
#include "wiki_socket.h"
#include "wiki_data.h"
#include "wiki_index.h"
#include "wiki_math.h"
#include "wiki_image.h"
#include "wiki_full_index.h"
#include "wiki_history.h"

#define MAX_PTHREAD_TOTAL 5

#define NOT_FOUND "Not found."

#define MAX_TRANS_BUF_SIZE (128*1024)

struct content_split {
	int start;
	int len;
};

struct lang_list {
	char lang[32];
};

struct one_lang {
	int flag;
	char lang[64];
	data_head_t data_head;
	WikiData *data;
	WikiIndex *index;
	WikiMath *math;
	WikiImage *image;
	WikiFullIndex *full_index;
};

#define MAX_PAGE_IDX_TOTAL 10000

#define MAX_TMP_HISTORY_TOTAL 4096
#define MAX_TMP_FAVORITE_TOTAL 4096

struct tmp_history {
	history_key_t *data;
	int read_time;
};

struct tmp_favorite {
	struct fav_key *data;
	struct fav_value value;
};

typedef struct {
	char title[256];
	char key[128];
	sort_idx_t idx;
	struct one_lang *which;
} wiki_title_t;

struct page_pos {
	int start;
	int end;
	struct one_lang *which;
};

typedef struct {
	char key[256];
	int *page_idx;
	int curr_page;
	int page_total;
	int all_total;
	struct page_pos page_pos[MAX_SELECT_LANG_TOTAL];
	int pos_total;
} full_search_t;

#define MAX_MATCH_TITLE_TOTAL (MAX_FIND_RECORD * 10 + 1)

#define CURR_WIKI(x) m_curr_lang->x

#define for_each_select_lang(x) \
	wiki_init_all_lang(); \
	struct one_lang *x; \
	for (int i = 0; i < m_select_lang_total && (x = wiki_get_lang_addr(m_select_lang[i])) != NULL; i++)

class WikiManage {
	private:

		WikiConfig *m_wiki_config;
		WikiFavorite *m_wiki_favorite;
		WikiSocket *m_wiki_socket;

		pthread_mutex_t m_mutex;

		WikiHistory *m_wiki_history;

		struct file_st *m_file_st;
		int m_file_st_total;

		struct one_lang m_all_lang[MAX_LANG_TOTAL];
		int m_all_lang_total; /* m_all_lang total */

		char m_select_lang[MAX_SELECT_LANG_TOTAL][24];
		int m_select_lang_total;

		struct one_lang *m_curr_lang; /* m_all_lang index */

		char *m_trans_buf;

#ifdef _USE_ZH
		WikiZh *m_wiki_zh;
#endif

		char *m_curr_content;
		char *m_curr_page;
		struct content_split m_split_pos[32];

		char *m_math_data[MAX_PTHREAD_TOTAL];

		wiki_title_t *m_match_title; /* save title list */
		int m_match_title_total;
		int m_match_title_flag; /* =0 one language, >0 more language */

		char m_curr_match_key[256];

		char m_curr_title[128];
		char *m_buf; /* 32K */
		int m_buf_len;

		struct tmp_history *m_history;
		int m_history_total;

		struct tmp_favorite *m_favorite;
		int m_favorite_total;

		full_search_t m_search_buf;

		int m_init_flag;

	public:
		WikiManage();
		~WikiManage();

		WikiConfig *wiki_get_config();

		int wiki_init();
		int wiki_reinit();
		int wiki_exit();

		int wiki_lang_init();
		int wiki_add_lang(struct one_lang *p, const struct file_st *f);
		int wiki_init_one_lang(const char *lang, int flag = 1);

		int wiki_select_lang(char lang[MAX_SELECT_LANG_TOTAL][24], int *total);

		struct one_lang *wiki_init_one_lang2(const struct file_st *f);
		struct file_st *wiki_get_file_st(const char *lang);

		int wiki_curr_lang(char *buf);
		int wiki_get_lang_list(struct lang_list *l, int *ret_total);
		int wiki_set_select_lang(int *idx, int len);

		int wiki_base_url(char *buf);
		int wiki_do_url(void *type, int sock, HttpParse *http, int idx);

		int wiki_split_content(int len);

		int wiki_read_data(const sort_idx_t *p, char **buf, const char *title);
		int wiki_match(const char *start, wiki_title_t **buf, int *total);
		int wiki_match_total();
		int wiki_match_lang(wiki_title_t **buf, int *total, int *flag);

		int wiki_random_history(char **buf, int *size);
		int wiki_random_favorite(char **buf, int *size);
		int wiki_random_select_lang(char **buf, int *size);
		int wiki_random_all_lang(char **buf, int *size);
		int wiki_random_page(char **buf, int *size);

		int wiki_curr_date_home_page(char **buf, int *size);
		int wiki_index_msg(char **buf, int *size);
		int wiki_add_key_pos(int pos, int height, int screen_height);
		int wiki_find_key_pos(int *pos, int *height);

		char *wiki_about();

		int wiki_get_title_from_url(char idx[16][256], char title[16][256], const char *buf);
		int wiki_view_index(int idx, char **buf, int *size);
		int wiki_not_found(char **buf, int *size, const char *str);

		int wiki_href(int href_idx, char **buf, int *size, const char *title);
		int wiki_find_key(const char *key, char **buf, int *size);
		int wiki_parse_url_local(const char *url, char *flag, char *ret_title, char **data);
		int wiki_parse_url(const char *url, char *flag, char *title, char **data);
		int wiki_parse_url_lang(const char *url, char *flag, char *title, char **data);

		char *wiki_curr_title();

		int wiki_push_back(int status, const char *title, int size, const sort_idx_t *idx);
		int wiki_curr(char **buf, int *size);
		int wiki_back(char *status, char **buf, int *size);
		int wiki_forward(char *status, char **buf, int *size);

		int wiki_history_begin();
		int wiki_history_next(int idx, char *title);

		int wiki_view_history(int idx, char **buf, int *size);
		int wiki_delete_history(int idx);
		int wiki_clean_history();

		int wiki_load_one_page_done();

		int wiki_view_favorite(int idx, char **buf, int *size);
		int wiki_favorite_rate(const char *title, const char *lang, char *percent);

		int wiki_favorite_begin();
		int wiki_favorite_next(int idx, char *title, char *rate);
		int wiki_favorite_total();

		int wiki_is_favorite();
		int wiki_add_favorite();
		int wiki_delete_favorite();
		int wiki_delete_favorite_one(int idx);
		int wiki_clean_favorite();

		struct one_lang *wiki_get_lang_addr(const char *lang);
		int wiki_translate_find(struct one_lang *p, const char *key, char *buf);
		int wiki_translate2(struct one_lang *p, const char *key, int len, char *buf);
		int wiki_translate_format(const char *buf, int len, char *to);
		int wiki_sys_translate(const char *key, const char *lang, char **buf);
		int wiki_translate(const char *key, char **buf);

		int wiki_page_last_read(char **buf, int *size);
		int wiki_scan_sdcard();

		int wiki_full_search(const char *key, char **buf, int *size, int page);
		int wiki_full_search_one_page(char **buf, int *size, int page);
		struct one_lang *wiki_full_search_find_lang(int idx);
		int wiki_check_full_search(const char *key);
		int wiki_init_all_lang();
		int wiki_full_search_update(const char *key);
		int wiki_parse_url_full_search(const char *url, char *flag, char *title, char **data);
};

#endif

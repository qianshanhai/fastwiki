/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __WIKI_MANAGE_H
#define __WIKI_MANAGE_H

#include "wiki_config.h"
#include "wiki_favorite.h"
#include "wiki_socket.h"
#include "wiki_data.h"
#include "wiki_index.h"
#include "wiki_math.h"
#include "wiki_image.h"
#include "wiki_history.h"

#define MAX_LANG_TOTAL 64
#define MAX_PTHREAD_TOTAL 5

/* status */
#define STATUS_MATCH_KEY 0x1
#define STATUS_CONTENT 0x2
#define STATUS_NOT_FOUND 0x4

#define NOT_FOUND "Not found."

struct content_split {
	int start;
	int len;
};

struct lang_list {
	char lang[32];
};

struct one_lang {
	WikiData *data;
	WikiIndex *index;
	WikiMath *math;
	WikiImage *image;
	data_head_t data_head;
	char lang[64];
	int flag;
};

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
} wiki_title_t;

#define CURR_WIKI(x) \
		({ \
		 	struct one_lang *p = &m_all_lang[m_curr_lang_idx]; \
			 if (strcmp(p->lang, m_curr_lang) != 0) { \
		 		for (int i = 0; i < m_all_lang_total; i++) { \
		 			p = &m_all_lang[i]; \
		 			if (strcmp(p->lang, m_curr_lang) == 0 && p->flag == 1) { \
						break; \
					} \
				} \
			} \
			m_curr_lang_idx >= 0 && p->flag == 1 ? (p->x) : NULL; \
		})

#define CURR_DATA_HEAD() \
		({ \
			 struct one_lang *p = &m_all_lang[m_curr_lang_idx]; \
		 	&p->data_head; \
		 })


class WikiManage {
	private:

		WikiConfig *m_wiki_config;
		WikiFavorite *m_wiki_favorite;
		WikiSocket *m_wiki_socket;
		WikiHistory *m_wiki_history;

		struct one_lang m_all_lang[MAX_LANG_TOTAL];
		int m_curr_lang_idx;  /* m_all_lang index */
		int m_all_lang_total; /* m_all_lang total */
		char m_curr_lang[64];

#ifdef _USE_ZH
		WikiZh *m_wiki_zh;
#endif

		char *m_curr_content;
		char *m_curr_page;
		struct content_split m_split_pos[32];

		char *m_math_data[MAX_PTHREAD_TOTAL];

		wiki_title_t *m_match_title; /* save title list */
		char m_curr_match_key[256];

		sort_idx_t *m_sort_idx;
		int m_sort_idx_total;

		char m_curr_title[128];
		char *m_buf; /* 32K */
		int m_buf_len;

		struct tmp_history *m_history;
		int m_history_total;

		struct tmp_favorite *m_favorite;
		int m_favorite_total;

		int m_init_flag;

	public:
		WikiManage();
		~WikiManage();

		int wiki_init();
		int wiki_reinit();
		int wiki_exit();

		int wiki_add_lang(struct one_lang *p, const struct file_st *f);
		int wiki_init_one_lang(const char *lang, int flag = 1);
		int wiki_init_one_lang(const struct file_st *f);
		int wiki_lang_init();

		int wiki_curr_lang(char *buf);
		int wiki_get_lang_list(struct lang_list *l, int *ret_total);

		int wiki_base_url(char *buf);
		int wiki_do_url(void *type, int sock, const char *url, int idx);

		int wiki_conv_key(const char *start, const char *key, int key_len, char *buf);
		int wiki_split_content(int len);

		int wiki_read_data(const sort_idx_t *p, char **buf, const char *title);
		int wiki_match(const char *start, wiki_title_t **buf, int *total);

		int wiki_random_page(char **buf, int *size);
		int wiki_index_msg(char **buf, int *size);
		int wiki_add_key_pos(int pos, int height, int screen_height);
		int wiki_find_key_pos(int *pos, int *height);
		int wiki_add_dir(const char *dir);

		int wiki_get_fontsize();
		int wiki_set_fontsize(int fontsize);

		int wiki_is_random();
		int wiki_switch_random();

		char *wiki_about();

		int wiki_get_hide_menu_flag();
		int wiki_set_hide_menu_flag();

		int wiki_set_color_mode();
		int wiki_get_color_mode();
		int wiki_get_list_color(char *list_bg, char *list_fg);

		int wiki_get_title_from_url(char idx[16][256], char title[16][256], const char *buf);
		int wiki_view_index(int idx, char **buf, int *size);
		int wiki_not_found(char **buf, int *size, const char *str);

		int wiki_href(int href_idx, char **buf, int *size, const char *title);
		int wiki_find_key(const char *key, char **buf, int *size);
		int wiki_parse_url_local(const char *url, char *flag, char *ret_title, char **data);
		int wiki_parse_url(const char *url, char *flag, char *title, char **data);

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

		int wiki_get_full_screen_flag();
		int wiki_switch_full_screen();

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

};

#endif

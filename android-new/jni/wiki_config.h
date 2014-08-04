/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __WIKI_CONFIG_H
#define __WIKI_CONFIG_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "s_hash.h"

#include "wiki_common.h"
#include "wiki-httpd.h"

#define BASE_DIR "/sdcard/hace/fastwiki"
#define WK_DAT_PREFIX "fastwiki.dat."
#define WK_IDX_PREFIX "fastwiki.idx."
#define WK_MATH_PREFIX "fastwiki.math."
#define WK_IMAGE_PREFIX "fastwiki.image."

#define LOG_FILE BASE_DIR "/" "fastwiki.log"

#define CFG_NEW_FILE "fastwiki3.cfg"

#define FW_VIEW_SIZE_G  (1024*1024*1024)

#define CHECK_CONFIG_SIZE() \
	do { \
		if (sizeof(fw_config_t) >= SHASH_USER_DATA_LEN) { \
			printf("error: sizeof(fw_config_t) = %d >> USER_DATA_LEN=%d\n", (int)sizeof(fw_config_t), (int)SHM_HASH_USER_DATA_LEN); \
			fflush(stdout); \
			exit(0); \
		} \
	} while (0)
			
typedef struct {
	unsigned char r;
	unsigned char g;
	unsigned char b;
} rgb_t;

inline char *rgb2str(char *buf, const rgb_t *p)
{
	sprintf(buf, "#%02X%02X%02X", (int)p->r, (int)p->g, (int)p->b);

	return buf;
}

inline rgb_t *str2rgb(rgb_t *p, const char *s)
{
	if (*s == '#')
		s++;

	p->r = hex2ch(s);
	p->g = hex2ch(s + 2);
	p->b = hex2ch(s + 4);

	return p;
}

typedef struct {
	rgb_t bg;
	rgb_t fg;
	rgb_t link;
	rgb_t list_bg;
	rgb_t list_fg;
	rgb_t reserve[1+2];
} color_t;

#define MAX_COLOR_TOTAL 8
#define MAX_FD_TOTAL 32
#define MAX_TMP_DIR_TOTAL 128
#define MAX_FW_DIR_TOTAL 10
#define MAX_FW_DIR_LEN   80
#define MAX_SELECT_LANG_TOTAL 30

enum {
	HOME_PAGE_BLANK = 0,
	HOME_PAGE_LAST_READ,
	HOME_PAGE_CURR_DATE,
};

enum {
	RANDOM_HISTORY = 0,
	RANDOM_FAVORITE,
	RANDOM_SELECT_LANG,
	RANDOM_ALL_LANG,
};

typedef struct {
	int version;
	unsigned int crc32;
	char select_lang[MAX_SELECT_LANG_TOTAL][24];
	int  select_lang_total;
	int  mutil_lang_list_mode; /* =0 output sort by lang, =1 output sort by title */ 
	char use_language[24];
	char r0[48];
	int font_size;
	unsigned int view_size_g; /* with G */
	unsigned int view_size_byte; /* with byte */
	unsigned int view_total; 
	unsigned int home_page; /* HOME_PAGE_ */
	unsigned int random_flag; /* RANDOM_ */
	char r1[40];
	int hide_menu_flag; /* =1 hide, =0 show */
	int color_index; /* 0 = */
	int color_total;
	color_t color[MAX_COLOR_TOTAL];
	int full_screen_flag; /* 1 = full screen */

	int translate_flag;   /* =1 enable, =0 disable */
	int translate_show_line; /* default = 6 */
	char default_trans_lang[24];
	int dir_total;
	char base_dir[MAX_FW_DIR_TOTAL][MAX_FW_DIR_LEN];
} fw_config_t;

struct fw_cfg_key {
	unsigned int crc32;
	unsigned int r_crc32;
};

struct fw_cfg_value {
	int pos;
	int height;
	int screen_height;
	unsigned char read_total;
	char reserve[7];
};

typedef char fw_files_t[MAX_FD_TOTAL][128];

struct file_st {
	fw_files_t data_file;
	int data_total;

	char index_file[128];
	char math_file[128];

	fw_files_t image_file;
	int image_total;

	int flag; /* =1 valid, =0 invalid */

	char lang[32];
};

#define MAX_LANG_TOTAL 128

typedef char tmp_dir_t[MAX_TMP_DIR_TOTAL][MAX_FW_DIR_LEN];

class WikiConfig;

class WikiConfig {
	private:
		fw_config_t *m_config;
		SHash *m_hash;

		int m_read_dir_count;

		struct file_st m_file[MAX_LANG_TOTAL];
		int m_file_total;

		tmp_dir_t m_tmp_dir;
		int m_tmp_dir_total;

	public:
		WikiConfig();
		~WikiConfig();

		int wc_init(const char *dir);
		int wc_get_fontsize();
		int wc_set_fontsize(int fontsize);
		int wc_add_dir(const char *dir);

		int wc_check_lang();
		int wc_get_lang_list(char lang[MAX_SELECT_LANG_TOTAL][24], int *lang_total, struct file_st **ret = NULL, int *total = NULL);

		int wc_add_key_pos(const char *title, int len,
					int pos, int height, int screen_height);
		int wc_find_key_pos(const char *title, int len, int *height, int *screen_height = NULL);

		void wc_add_view_size(int size);
		void wc_add_view_total(int total = 1);

		int wc_init_one_lang(const char *lang);

		int wc_get_hide_menu_flag();
		int wc_set_hide_menu_flag();

		int wc_init_color();
		int wc_add_color(const char *bg, const char *fg, const char *link,
					const char *list_bg, const char *list_fg);

		int wc_get_color_mode();
		int wc_set_color_mode(int idx);

		int wc_get_list_color(char *list_bg, char *list_fg);

		int wc_get_config(char bg[16], char fg[16], char link[16], int *font_size);

		int wc_get_full_screen_flag();
		int wc_switch_full_screen();

		int wc_set_default_lang(const char lang[MAX_SELECT_LANG_TOTAL][24], int total);

		int wc_get_mutil_lang_list_mode();
		int wc_set_mutil_lang_list_mode(int mode);

		int wc_get_translate_show_line();
		int wc_set_translate_show_line(int x);

		int wc_get_use_language();
		int wc_set_use_language(int idx);

		int wc_get_home_page_flag();
		int wc_set_home_page_flag(int mode);

		int wc_get_need_translate();
		int wc_set_need_translate(int flag);

		int wc_get_translate_default(char *lang);
		int wc_set_translate_default(const char *lang);

		int wc_merge_tmp_dir_first(int flag, tmp_dir_t tmp, int tmp_total, tmp_dir_t to, int *to_total,
					int (*cmp)(const char *a, const char *b, int len, char *buf));
		int wc_merge_tmp_dir();
		int wc_clean_tmp_dir();
		int wc_add_tmp_dir(const char *dir);
		int wc_scan_all();
		int wc_scan_sdcard(const char *dir, int flag);

		int wc_get_random_flag();
		int wc_set_random_flag(int mode);

	private:
		int wc_init_config(const char *file);
		int wc_init_lang();

		struct file_st *wc_get_file_st(const char *lang);
		int wc_scan_lang(const char *dir, const char *fname);
		int wc_scan_dir(const char *dir, 
					int (WikiConfig::*func)(const char *dir, const char *fname));
		int wc_add_lang(int flag_idx, const char *lang);
};

extern "C" {
	int LOG(const char *fmt, ...);
};

#endif

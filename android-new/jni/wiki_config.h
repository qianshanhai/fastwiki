/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __WIKI_CONFIG_H
#define __WIKI_CONFIG_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "s_hash.h"

#include "wiki_bookmark.h"
#include "wiki_scan_file.h"
#include "wiki_common.h"
#include "wiki-httpd.h"

#define BASE_DIR "/sdcard/hace/fastwiki"
#define LOG_FILE BASE_DIR "/" "fastwiki.log"

#define CFG_NEW_FILE "fastwiki32.cfg"

#define FW_VIEW_SIZE_G  (1024*1024*1024)

typedef struct {
	char bg[8];
	char fg[8];
	char link[8];
	char list_bg[8];
	char list_fg[8];
	char reserve[64];
} color_t;

#define MAX_COLOR_TOTAL 16
#define MAX_SELECT_LANG_TOTAL 128
#define MAX_LANG_TOTAL 128

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

enum {
	FULL_TEXT_SHOW_SOME = 0,
	FULL_TEXT_SHOW_TITLE,
	FULL_TEXT_SHOW_ALL,
};

typedef struct {
	int version;
	unsigned int crc32;
	int font_size;
	int  mutil_lang_list_mode; /* =0 output sort by lang, =1 output sort by title */ 
	int hide_menu_flag; /* =1 hide, =0 show */
	unsigned int view_size_g; /* with G */
	unsigned int view_size_byte; /* with byte */
	unsigned int view_total; 
	unsigned int home_page; /* HOME_PAGE_ */
	unsigned int random_flag; /* RANDOM_ */
	unsigned int body_image_flag; /* =1 use body image, =0 none */
	int full_screen_flag; /* 1 = full screen */
	int translate_flag;   /* =1 enable, =0 disable */
	int translate_show_line; /* default = 6 */
	int color_index; /* 0 = */
	int color_total;
	color_t color[MAX_COLOR_TOTAL];
	char default_trans_lang[24];
	char body_image_path[128];
	fw_dir_t base_dir[256];
	int dir_total;
	char select_lang[MAX_SELECT_LANG_TOTAL][24];
	int  select_lang_total;
	char use_language[24];
	int full_text_show;
	int audio_flag; 
	char audio_path[256];
	char r[10*1024 - 4 - 256];
} fw_config_t;

class WikiConfig {
	private:
		fw_config_t *m_config;
		WikiBookmark *m_bookmark;

		struct file_st m_file[MAX_LANG_TOTAL];
		int m_file_total;

		WikiScanFile *m_scan_file; 
	public:
		WikiConfig();
		~WikiConfig();

		int wc_init();
		int wc_get_fontsize();
		int wc_set_fontsize(int fontsize);
		int wc_add_dir(const char *dir);

		int wc_check_lang();
		int wc_get_lang_list(char lang[MAX_SELECT_LANG_TOTAL][24], int *lang_total, struct file_st **ret = NULL, int *total = NULL);

		int wc_add_key_pos(const char *title, int len, int pos, int height, int screen_height);
		int wc_find_key_pos(const char *title, int len, bookmark_value_t *v);

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

		int wc_scan_all();

		int wc_get_random_flag();
		int wc_set_random_flag(int mode);
		int wc_get_full_text_show();
		int wc_set_full_text_show(int idx);

		int wc_get_body_image_flag();
		int wc_set_body_image_flag(int flag);
		const char *wc_get_body_image_path();
		int wc_set_body_image_path(const char *path);

		int wc_set_audio_flag(int flag);
		int wc_get_audio_flag();
		const char *wc_get_audio_path();
		int wc_set_audio_path(const char *path);

		int wc_add_dir(const char *dir, int flag);
		int wc_get_dir(char *dir1, char *dir2);

	private:
		int wc_init_config();
		int wc_init_lang();
};

#endif

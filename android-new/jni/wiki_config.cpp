/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <stdarg.h>

#include "q_util.h"
#include "crc32sum.h"

#include "wiki_config.h"
#include "wiki_local.h"

#ifndef WIN32
int LOG(const char *fmt, ...)
{
	int n, now_n;
	char now[64], buf[1024];
	va_list ap;
	struct timeval tv;
	struct tm *tm;

	gettimeofday(&tv, NULL);
	tm = localtime(&tv.tv_sec);

	now_n = sprintf(now, "[%02d:%02d:%02d.%03d] ",
			(int)tm->tm_hour, (int)tm->tm_min, (int)tm->tm_sec, (int)(tv.tv_usec / 1000));

	va_start(ap, fmt);
	n = vsnprintf(buf + 64, sizeof(buf) - 64, fmt, ap);
	va_end(ap);

	memcpy(buf + 64 - now_n, now, now_n);

	file_append(LOG_FILE, buf + 64 - now_n, n + now_n);

	return n;
}
#endif

WikiConfig::WikiConfig()
{
	m_config = NULL;
}

WikiConfig::~WikiConfig()
{
	if (m_bookmark)
		delete m_bookmark;
}

/* TODO check return value */
int WikiConfig::wc_init()
{
	m_scan_file = new WikiScanFile();

	if (wc_init_config() == -1)
		return -1;

	if (m_config->use_language[0] == 0)
		set_lang("en");
	else
		set_lang(m_config->use_language);

	wc_init_lang();

	m_config->color_total = 0;
	wc_init_color();

	if (m_config->font_size <= 0)
		m_config->font_size = 13;

	return 0;
}

int WikiConfig::wc_get_fontsize()
{
	return m_config == NULL ? 13 : m_config->font_size;
}

int WikiConfig::wc_set_fontsize(int fontsize)
{
	if (m_config == NULL)
		return 13;

	int n = m_config->font_size;
	
	if (n <= 0)
		n = 13;

	if (n > 3 || fontsize > 0) {
		n += fontsize;
		m_config->font_size = n; 
	}

	return n;
}

int WikiConfig::wc_init_config()
{
	mapfile_t mt;
	char file[128];

	if (!dashd(BASE_DIR)) {
		if (q_mkdir(BASE_DIR, 0755) == -1)
			return -1;
	}

	sprintf(file, "%s/%s", BASE_DIR, CFG_NEW_FILE);

	if (!dashf(file)) {
		int fd = open(file, O_RDWR | O_CREAT, 0644);
		if (fd == -1)
			return -1;
		fw_config_t *p = (fw_config_t *)malloc(sizeof(fw_config_t));
		memset(p, 0, sizeof(fw_config_t));
		write(fd, p, sizeof(fw_config_t));
		close(fd);
	}

	if ((m_config = (fw_config_t *)mapfile(file, &mt, 1)) == NULL)
		return -1;;

	m_bookmark = new WikiBookmark();
	m_bookmark->wbm_init(BASE_DIR);

	return 0;
}

int WikiConfig::wc_init_lang()
{
	if (m_config->dir_total == 0) {
		memset(m_config->base_dir, 0, sizeof(m_config->base_dir));
		fw_dir_t *p = &m_config->base_dir[m_config->dir_total++];
		strcpy(p->path, BASE_DIR);
	}

	m_file_total = m_scan_file->wsf_scan_file(m_file, MAX_LANG_TOTAL, m_config->base_dir, m_config->dir_total);

	if (m_file_total == 0)
		return -1;

	int total = 0;
	char tmp[MAX_SELECT_LANG_TOTAL + 1][24];

	memset(tmp, 0, sizeof(tmp));

	for (int i = 0; i < m_config->select_lang_total; i++) {
		int found = 0;
		for (int j = 0; j < m_file_total; j++) {
			if (strcmp(m_config->select_lang[i], m_file[j].lang) == 0) {
				found = 1;
				break;
			}
		}
		if (found == 1) {
			strcpy(tmp[total++], m_config->select_lang[i]);
		}
	}

	if (total == 0) {
		strcpy(m_config->select_lang[0], m_file[0].lang);
		m_config->select_lang_total = 1;
	} else if (total < m_config->select_lang_total) {
		m_config->select_lang_total = total;
		memcpy(m_config->select_lang, tmp, sizeof(m_config->select_lang));
	}

	return 0;
}

int WikiConfig::wc_scan_all()
{
	char *dir[] = { "/storage", "/mnt", NULL};

	m_config->dir_total = m_scan_file->wsf_fetch_fw_dir(m_config->base_dir, sizeof(m_config->base_dir) / sizeof(fw_dir_t), dir, 2);
	wc_init_lang();

	if (m_file_total > 0)
		wc_set_translate_default(m_file[0].lang);

	return 0;
}

int WikiConfig::wc_check_lang()
{
	struct file_st *p;

	for (int i = 0; i < m_file_total; i++) {
		p = &m_file[i];
		if (p->index_file[0] && p->data_total > 0)
			p->flag = 1;
	}

	return 0;
}

int WikiConfig::wc_get_lang_list(char lang[MAX_SELECT_LANG_TOTAL][24], int *lang_total, struct file_st **ret, int *total)
{
	if (m_config == NULL)
		return 0;

	*lang_total = m_config->select_lang_total;

	memcpy(lang, m_config->select_lang, sizeof(m_config->select_lang));

	if (ret) {
		*ret = m_file;
		*total = m_file_total;
	}

	return 0;
}

#define my_abs(x) ((x) > 0 ? (x) : (-x))

int WikiConfig::wc_add_key_pos(const char *title, int len,
		int pos, int height, int screen_height)
{
	return m_bookmark->wbm_add_bookmark(title, len, pos, height, screen_height);
}

int WikiConfig::wc_find_key_pos(const char *title, int len, bookmark_value_t *v)
{
	return m_bookmark->wbm_find_bookmark(title, len, v);
}

#define G *1024*1024*1024

void WikiConfig::wc_add_view_size(int size)
{
	m_config->view_size_byte += size;

	if (m_config->view_size_byte >= (1 G)) {
		m_config->view_size_g += m_config->view_size_byte / (1 G);
		m_config->view_size_byte -= (1 G);
	}
}

void WikiConfig::wc_add_view_total(int total)
{
	m_config->view_total += total;
}

int WikiConfig::wc_get_hide_menu_flag()
{
	return m_config->hide_menu_flag;
}

int WikiConfig::wc_set_hide_menu_flag()
{
	if (m_config->hide_menu_flag == 0)
		m_config->hide_menu_flag = 1;
	else
		m_config->hide_menu_flag = 0;

	return m_config->hide_menu_flag;
}

int WikiConfig::wc_add_color(const char *bg, const char *fg, const char *link,
			const char *list_bg, const char *list_fg)
{
	color_t *p = &m_config->color[m_config->color_total];

	strcpy(p->bg, bg);
	strcpy(p->fg, fg);
	strcpy(p->link, link);
	strcpy(p->list_bg, list_bg);
	strcpy(p->list_fg, list_fg);

	m_config->color_total++;

	return 0;
}

const char *m_color[16] = {
	"#000000", /* black */
	"#808080",
	"#FFFFFF", /* white */
	"#0000FF", /* blue */
	"#A0A0A0",
};

int WikiConfig::wc_init_color()
{
	if (m_config->color_total == 0) {
		wc_add_color(m_color[2], m_color[0], m_color[3], m_color[2], m_color[0]);
		wc_add_color(m_color[0], m_color[1], m_color[4], m_color[0], m_color[4]);
	}

	return 0;
}

int WikiConfig::wc_get_color_mode()
{
	return m_config == NULL ? 0 : m_config->color_index;
}

int WikiConfig::wc_set_color_mode(int idx)
{
	m_config->color_index = idx;

	return idx;
}

int WikiConfig::wc_get_config(char bg[16], char fg[16], char link[16], int *font_size)
{
	if (m_config == NULL) {
		strcpy(bg, m_color[2]);
		strcpy(fg, m_color[0]);
		strcpy(link, m_color[3]);
		*font_size = 13;

		return 0;
	}

	color_t *p = &m_config->color[m_config->color_index];

	strcpy(bg, p->bg);
	strcpy(fg, p->fg);
	strcpy(link, p->link);

	*font_size = m_config->font_size;

	return 0;
}

int WikiConfig::wc_get_list_color(char *list_bg, char *list_fg)
{
	if (m_config == NULL) {
		strcpy(list_bg, m_color[2]);
		strcpy(list_fg, m_color[0]);
		return 0;
	}

	color_t *p = &m_config->color[m_config->color_index];

	strcpy(list_bg, p->list_bg);
	strcpy(list_fg, p->list_fg);

	return 0;
}

int WikiConfig::wc_get_full_screen_flag()
{
	return m_config == NULL ? 0 : m_config->full_screen_flag;
}

int WikiConfig::wc_switch_full_screen()
{
	if (m_config->full_screen_flag)
		m_config->full_screen_flag = 0;
	else
		m_config->full_screen_flag = 1;

	return m_config->full_screen_flag;
}

int WikiConfig::wc_set_default_lang(const char lang[MAX_SELECT_LANG_TOTAL][24], int total)
{
	m_config->select_lang_total = total;

	for (int i = 0; i < total; i++) {
		strncpy(m_config->select_lang[i], lang[i], sizeof(m_config->select_lang[i]) - 1);
	}

	return 0;
}

int WikiConfig::wc_get_mutil_lang_list_mode()
{
	return m_config->mutil_lang_list_mode;
}

int WikiConfig::wc_set_mutil_lang_list_mode(int mode)
{
	m_config->mutil_lang_list_mode = mode;

	return 0;
}

int WikiConfig::wc_get_home_page_flag()
{
	return m_config == NULL ? HOME_PAGE_BLANK : m_config->home_page;
}

int WikiConfig::wc_set_home_page_flag(int mode)
{
	m_config->home_page = mode;

	return mode;
}

int WikiConfig::wc_get_translate_show_line()
{
	int l = m_config->translate_show_line;

	if (l <= 1 || l >= 100) {
		l = 6;
		m_config->translate_show_line = l;
	}

	return l;
}

int WikiConfig::wc_set_translate_show_line(int x)
{
	m_config->translate_show_line += x;

	return wc_get_translate_show_line();
}

int WikiConfig::wc_get_use_language()
{
	if (m_config == NULL)
		return 0;

	return strcmp(m_config->use_language, "zh") == 0 ? 1 : 0;
}

int WikiConfig::wc_set_use_language(int idx)
{
	const char *lang[] = {"en", "zh", NULL};

	strcpy(m_config->use_language, lang[idx]);
	set_lang(lang[idx]);

	return idx;
}

int WikiConfig::wc_get_need_translate()
{
	return m_config->translate_flag;
}

int WikiConfig::wc_set_need_translate(int flag)
{
	m_config->translate_flag = flag;

	return flag;
}

int WikiConfig::wc_get_translate_default(char *lang)
{
	strcpy(lang, m_config->default_trans_lang);
	
	return 0;
}

int WikiConfig::wc_set_translate_default(const char *lang)
{
	strncpy(m_config->default_trans_lang, lang, sizeof(m_config->default_trans_lang) - 1);

	return 0;
}

int WikiConfig::wc_get_random_flag()
{
	return m_config->random_flag;
}

int WikiConfig::wc_set_random_flag(int mode)
{
	m_config->random_flag = mode;

	return mode;
}

int WikiConfig::wc_get_body_image_flag()
{
	return m_config == NULL ? 0 : m_config->body_image_flag;
}

int WikiConfig::wc_set_body_image_flag(int flag)
{
	m_config->body_image_flag = flag;

	return 0;
}

const char *WikiConfig::wc_get_body_image_path()
{
	return m_config->body_image_path;
}

int WikiConfig::wc_set_body_image_path(const char *path)
{
	memset(m_config->body_image_path, 0, sizeof(m_config->body_image_path));
	strncpy(m_config->body_image_path, path, sizeof(m_config->body_image_path) - 1);

	return 0;
}


int WikiConfig::wc_get_full_text_show()
{
	return m_config->full_text_show;
}

int WikiConfig::wc_set_full_text_show(int idx)
{
	m_config->full_text_show = idx;

	return idx;
}

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

WikiConfig::WikiConfig()
{
	m_config = NULL;
}

WikiConfig::~WikiConfig()
{
	if (m_hash)
		delete m_hash;
}

/* TODO check return value */
int WikiConfig::wc_init(const char *dir)
{
	int r;
	char file[256];

	if (dir != NULL && dir[0] == '.') {
		strcpy(file, CFG_NEW_FILE);
	} else {
		if (!dashd(BASE_DIR)) {
			if (q_mkdir(BASE_DIR, 0755) == -1)
				return -1;
		}

		sprintf(file, "%s/%s", BASE_DIR, CFG_NEW_FILE);
	}

	r = dashf(file);
	wc_init_config(file);

	if (!r)
		wc_add_dir(BASE_DIR);

	if (dir)
		wc_add_dir(dir);

	wc_init_lang();

	m_config->color_total = 0;
	wc_init_color();

	if (wc_get_fontsize() <= 0)
		wc_set_fontsize(12);

	return 0;
}

int WikiConfig::wc_get_fontsize()
{
	return m_config == NULL ? 0 : m_config->font_size;
}

int WikiConfig::wc_set_fontsize(int fontsize)
{
	if (m_config == NULL)
		return 0;

	m_config->font_size = fontsize <= 0 ? 13 : fontsize;

	return 1;
}

int WikiConfig::wc_init_config(const char *file)
{
	m_hash = new SHash();

	m_hash->sh_set_hash_magic(10000);

#ifdef FW_NJI
	if (m_hash->sh_init(file, sizeof(struct fw_cfg_key), sizeof(struct fw_cfg_value), 1000, 1) == -1)
		return -1;
#else
	if (m_hash->sh_init(100, sizeof(struct fw_cfg_key), sizeof(struct fw_cfg_value)) == -1)
		return -1;
#endif

	m_hash->sh_get_user_data((void **)&m_config);

	if (m_config->version == 0) {
		m_hash->sh_clean();
		m_config->version = 1;
	}

	return 0;
}

int WikiConfig::wc_init_lang()
{
	m_file_total = 0;
	memset(m_file, 0, sizeof(m_file));

	for (int i = 0; i < m_config->dir_total; i++) {
		m_read_dir_count = 0;
		wc_scan_dir(m_config->base_dir[i], &WikiConfig::wc_scan_lang);
	}

	if (m_file_total == 0)
		return -1;
	
	if (m_config->default_lang[0] == 0) {
		strcpy(m_config->default_lang, m_file[0].lang);
		return 0;
	}

	int found = 0;

	for (int i = 0; i < m_file_total; i++) {
		struct file_st *p = &m_file[i];
		if (strcmp(p->lang, m_config->default_lang) == 0) {
			found = 1;
			break;
		}
	}

	if (found == 0)
		strcpy(m_config->default_lang, m_file[0].lang);

	return 0;
}

/*
 */
int WikiConfig::wc_add_dir(const char *dir)
{
#if 1
	char buf[256];

	strcpy(buf, m_config->base_dir[0]);

	strncpy(m_config->base_dir[0], dir, MAX_FW_DIR_LEN - 1);
	m_config->dir_total = 1;

	if (wc_init_lang() == -1) {
		strncpy(m_config->base_dir[0], buf, MAX_FW_DIR_LEN - 1);
		wc_init_lang();
		return -1;
	}

	return 0;
#endif

	if (m_config->dir_total >= MAX_FW_DIR_TOTAL - 1)
		return 1;

	for (int i = 0; i < m_config->dir_total; i++) {
		if (strncmp(m_config->base_dir[i], dir, strlen(dir)) == 0)
			return 2;
		if (strncmp(dir, m_config->base_dir[i], strlen(m_config->base_dir[i])) == 0) {
			strncpy(m_config->base_dir[i], dir, MAX_FW_DIR_LEN - 1);
			return 3;
		}
	}

	strncpy(m_config->base_dir[m_config->dir_total++], dir, MAX_FW_DIR_LEN - 1);

	return 0;
}

#define my_strncmp(s, STR) strncmp(s, STR, sizeof(STR) - 1)

struct file_st *WikiConfig::wc_get_file_st(const char *lang)
{
	struct file_st *p;

	for (int i = 0; i < m_file_total; i++) {
		p = &m_file[i];
		if (strcmp(p->lang, lang) == 0) {
			strncpy(p->lang, lang, sizeof(p->lang) - 1);
			return p;
		}
	}

	p = &m_file[m_file_total++];
	strncpy(p->lang, lang, sizeof(p->lang) - 1);

	return p;
}

int WikiConfig::wc_scan_lang(const char *dir, const char *fname)
{
	split_t sp;
	struct file_st *p;

	if (split('.', fname, sp) >= 3) {
		if (my_strncmp(fname, WK_DAT_PREFIX) == 0) {
			p = wc_get_file_st(sp[2]);
			sprintf(p->data_file[p->data_total++], "%s/%s", dir, fname);
		} else if (my_strncmp(fname, WK_IDX_PREFIX) == 0) {
			p = wc_get_file_st(sp[2]);
			sprintf(p->index_file, "%s/%s", dir, fname);
		} else if (my_strncmp(fname, WK_MATH_PREFIX) == 0) {
			p = wc_get_file_st(sp[2]);
			sprintf(p->math_file, "%s/%s", dir, fname);
		} else if (my_strncmp(fname, WK_IMAGE_PREFIX) == 0) {
			p = wc_get_file_st(sp[2]);
			sprintf(p->image_file[p->image_total++], "%s/%s", dir, fname);
		}
	}

	return 0;
}

int WikiConfig::wc_scan_dir(const char *dir, 
		int (WikiConfig::*func)(const char *dir, const char *fname))
{
	DIR *dirp;
	struct dirent *d;
	char buf[1024];
	int n = 0;
	int deep = 2;

	if (strncmp(dir, BASE_DIR, sizeof(BASE_DIR) - 1) == 0)
		deep = 5;

	if (m_read_dir_count >= deep)
		return 0;

	m_read_dir_count++;

	if ((dirp = opendir(dir)) == NULL)
		return -1;

	while ((d = readdir(dirp))) {
		if (d->d_name[0] == '.')
			continue;
		snprintf(buf, sizeof(buf), "%s/%s", dir, d->d_name);
		if (dashd(buf)) {
			if ((n = wc_scan_dir(buf, func)) == -1) {
				break;
			}
		} else {
			if ((n = (this->*func)(dir, d->d_name)) == -1)
				break;
		}
	}

	closedir(dirp);
	
	m_read_dir_count--;

	return n;
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

int WikiConfig::wc_get_lang_list(char *default_lang, struct file_st **ret, int *total)
{
	strcpy(default_lang, m_config->default_lang);

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
	struct fw_cfg_key key;
	struct fw_cfg_value value, *f;

	crc32sum(title, len, &key.crc32, &key.r_crc32);

#ifdef FW_DEBUG
	LOG("add key=%u[%s], pos=%d\n", key.crc32, title, pos);
#endif

	if (m_hash->sh_find(&key, (void **)&f) == _SHASH_FOUND) {
		if (pos > 0) {
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

int WikiConfig::wc_find_key_pos(const char *title, int len, int *height,
		int *screen_height)
{
	struct fw_cfg_key key;
	struct fw_cfg_value *f;

	*height = 1;
	if (screen_height)
		*screen_height = 0;

	crc32sum(title, len, &key.crc32, &key.r_crc32);

	if (m_hash->sh_find(&key, (void **)&f) == _SHASH_FOUND) {
#ifdef FW_DEBUG
		LOG("find key=%u[%s], pos=%d\n", key.crc32, title, f->pos);
#endif
		if (f->pos < 0 || f->height <= 0)
			return 0;

		*height = f->height;
		if (screen_height)
			*screen_height = f->screen_height;

		return  f->pos;
	}

	return 0;
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

void WikiConfig::wc_set_random()
{
	if (m_config->random_flag == 0)
		m_config->random_flag = 1;
	else
		m_config->random_flag = 0;
}

int WikiConfig::wc_is_random()
{
	return m_config->random_flag;
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

	str2rgb(&p->bg, bg);
	str2rgb(&p->fg, fg);
	str2rgb(&p->link, link);
	str2rgb(&p->list_bg, list_bg);
	str2rgb(&p->list_fg, list_fg);

	m_config->color_total++;

	return 0;
}

int WikiConfig::wc_init_color()
{
	const char *color[16] = {
		"#000000", /* black */
		"#808080",
		"#FFFFFF", /* white */
		"#0000FF", /* blue */
		"#A0A0A0",
	};

	if (m_config->color_total == 0) {
		wc_add_color(color[2], color[0], color[3], color[2], color[0]);
		wc_add_color(color[0], color[1], color[4], color[0], color[4]);
	}

	return 0;
}

int WikiConfig::wc_get_color_mode()
{
	return m_config->color_index;
}

int WikiConfig::wc_set_color_mode()
{
	if (m_config->color_index == 0)
		m_config->color_index = 1;
	else
		m_config->color_index = 0;

	return m_config->color_index;
}

int WikiConfig::wc_get_config(char bg[16], char fg[16], char link[16], int *font_size)
{
	color_t *p = &m_config->color[m_config->color_index];

	rgb2str(bg, &p->bg);
	rgb2str(fg, &p->fg);
	rgb2str(link, &p->link);

	*font_size = m_config->font_size;

	return 0;
}

int WikiConfig::wc_get_list_color(char *list_bg, char *list_fg)
{
	color_t *p = &m_config->color[m_config->color_index];
	
	rgb2str(list_bg, &p->list_bg);
	rgb2str(list_fg, &p->list_fg);

	return 0;
}

int WikiConfig::wc_get_full_screen_flag()
{
	return m_config->full_screen_flag;
}

int WikiConfig::wc_switch_full_screen()
{
	if (m_config->full_screen_flag)
		m_config->full_screen_flag = 0;
	else
		m_config->full_screen_flag = 1;

	return m_config->full_screen_flag;
}

int WikiConfig::wc_set_default_lang(const char *lang)
{
	strncpy(m_config->default_lang, lang, sizeof(m_config->default_lang) - 1);

	return 0;
}

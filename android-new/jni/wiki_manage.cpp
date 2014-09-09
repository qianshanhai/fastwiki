/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>

#include "wiki_manage.h"
#include "wiki_common.h"
#include "wiki_local.h"

#define INIT_CURR_TITLE()   m_curr_title[0] = 0
#define SET_CURR_TITLE(key) strcpy(m_curr_title, key)

static int _wiki_do_url(void *_class, void *type, int sock, const char *url, int idx)
{
	WikiManage *manage = (WikiManage *)_class;
	
	return manage->wiki_do_url(type, sock, url, idx);
}

#define INIT_CURR_LANG() m_curr_lang = NULL

WikiManage::WikiManage()
{
	m_wiki_config = NULL;
	m_wiki_favorite = NULL;
	m_wiki_socket = NULL;
	m_wiki_history = NULL;

#ifdef _USE_ZH
	m_wiki_zh = NULL;
#endif

	INIT_CURR_LANG();

	m_init_flag = 0;
}

WikiManage::~WikiManage()
{
}

static void *wiki_manage_start_socket_thread(void *arg)
{
	WikiSocket *ws = (WikiSocket *)arg;

	ws->ws_start_http_server(MAX_PTHREAD_TOTAL);

	return NULL;
}

int WikiManage::wiki_init()
{

	struct timeval now;

	gettimeofday(&now, NULL);
	srand(now.tv_sec | now.tv_usec);

	m_buf = (char *)malloc(32*1024);
	m_trans_buf = (char *)malloc(MAX_TRANS_BUF_SIZE);

	m_curr_content = (char *)malloc(2*1024*1024);
	m_curr_page = (char *)malloc(2*1024*1024);

	memset(&m_all_lang[0], 0, sizeof(struct one_lang));

	m_match_title = (wiki_title_t *)calloc(MAX_MATCH_TITLE_TOTAL + 1, sizeof(wiki_title_t));
	memset(m_match_title, 0, (MAX_MATCH_TITLE_TOTAL + 1) * sizeof(wiki_title_t));

	for (int i = 0; i < MAX_PTHREAD_TOTAL; i++) {
		m_math_data[i] = (char *)malloc(512*1024);
	}

	m_wiki_config = new WikiConfig();
	m_wiki_config->wc_init(NULL);

	m_wiki_socket = new WikiSocket();
	m_wiki_socket->ws_init(NULL, _wiki_do_url, (void *)this);

	pthread_t id;
	pthread_create(&id, NULL, wiki_manage_start_socket_thread, m_wiki_socket);
	usleep(10000);

	m_wiki_history = new WikiHistory();
	m_wiki_history->wh_init();
	m_history = (struct tmp_history *)calloc(MAX_TMP_HISTORY_TOTAL + 10, sizeof(struct tmp_history));

	m_wiki_favorite = new WikiFavorite();
	m_wiki_favorite->wf_init();
	m_favorite = (struct tmp_favorite *)calloc(MAX_TMP_FAVORITE_TOTAL + 10, sizeof(struct tmp_favorite));

	INIT_CURR_LANG();

	pthread_mutex_init(&m_mutex, NULL);

	if (wiki_lang_init() == -1)
		return -1;

	m_init_flag = 1;

	return 0;
}

int WikiManage::wiki_reinit()
{
	struct one_lang *p;
	
	m_init_flag = 0;

	for (int i = 0; i < m_all_lang_total; i++) {
		p = &m_all_lang[i];
		if (p->flag == 0)
			continue;

		if (p->data)
			delete p->data;

		if (p->index)
			delete p->index;

		if (p->math)
			delete p->math;

		if (p->image)
			delete p->image;
	}

	INIT_CURR_LANG();

	m_wiki_config->wc_init(NULL);
	if (wiki_lang_init() == -1)
		return -1;

	m_init_flag = 1;

	return 0;
}

int WikiManage::wiki_exit()
{
	if (m_wiki_config)
		delete m_wiki_config;

	if (m_wiki_history)
		delete m_wiki_history;

	exit(0);

	return 0;
}

int WikiManage::wiki_add_lang(struct one_lang *p, const struct file_st *f)
{
	strcpy(p->lang, f->lang);

	p->data = new WikiData();
	p->index = new WikiIndex();
	p->math = new WikiMath();
	p->image = new WikiImage();

	if (p->data->wd_init(f->data_file, f->data_total) == -1) {
		for (int i = 0; i < f->data_total; i++) {
			LOG("Not found data file: %s\n", f->data_file[i]);
		}
		return -1;
	}
	p->data->wd_get_head(&p->data_head);

	p->index = new WikiIndex();
	if (p->index->wi_init(f->index_file) == -1) {
		LOG("Not found %s: %s\n", f->index_file, strerror(errno));
		return -1;
	}

	p->math->wm_init(f->math_file);
	p->image->we_init(f->image_file, f->image_total);

	p->flag = 1;

	return 0;
}

int WikiManage::wiki_init_one_lang(const char *lang, int flag)
{
	struct one_lang *l;
	struct file_st *f;

	if ((f = wiki_get_file_st(lang)) == NULL)
		return -1;

	if ((l = wiki_init_one_lang2(f)) == NULL)
		return -1;

	if (flag) {
		m_curr_lang = l;
	}

	return 0;
}

struct one_lang *WikiManage::wiki_init_one_lang2(const struct file_st *f)
{
	struct one_lang *p;

	for (int i = 0; i < m_all_lang_total; i++) {
		p = &m_all_lang[i];
		if (p->flag == 1 && strcmp(p->lang, f->lang) == 0) {
			return p;
		}
	}

	p = &m_all_lang[m_all_lang_total++];

	if (wiki_add_lang(p, f) == -1) {
		if (m_all_lang_total > 0)
			m_all_lang_total--;
		return NULL;
	}

	return p;
}

int WikiManage::wiki_lang_init()
{
	m_wiki_config->wc_get_lang_list(m_select_lang, &m_select_lang_total, &m_file_st, &m_file_st_total);

	if (m_file_st_total == 0)
		return -1;

	for (int i = 0; i < m_select_lang_total; i++) {
		wiki_init_one_lang(m_select_lang[i], 0);
	}
	/* TODO */

	return 0;
}

struct file_st *WikiManage::wiki_get_file_st(const char *lang)
{
	for (int i = 0; i < m_file_st_total; i++) {
		if (strcmp(m_file_st[i].lang, lang) == 0)
			return &m_file_st[i];
	}

	return NULL;
}

int WikiManage::wiki_get_lang_list(struct lang_list *l, int *ret_total)
{
	struct file_st *f;

	for (int i = 0; i < m_file_st_total; i++) { 
		f = &m_file_st[i];
		strcpy(l[i].lang, f->lang);
	}

	*ret_total = m_file_st_total;

	return m_file_st_total;
}

/*
 *  http://127.0.0.1:port/
 */
int WikiManage::wiki_base_url(char *buf)
{
	int port = 8218;

	if (m_wiki_socket)
		port = m_wiki_socket->ws_get_bind_port();

	sprintf(buf, "http://127.0.0.1:%d/",port); 

	return 0;
}

int WikiManage::wiki_do_url(void *type, int sock, const char *url, int idx)
{
	int len;
	char *data = m_math_data[idx];
	WikiSocket *ws;
	
	ws = (WikiSocket *)type;

	if (strncasecmp(url, "curr:", 5) == 0) {
		struct content_split *p = &m_split_pos[atoi(url + 5) + 1];
#if 0
		if ((len = m_wiki_zh->wz_convert_2hans(m_curr_content + p->start, p->len, data)) <= 0)
#endif
		{
			data = m_curr_content + p->start;
			len = p->len;
		}

		ws->ws_http_output_head(sock, 200, "text/html", len);
		ws->ws_http_output_body(sock, data, len);
#if 0
			int i;
			for (i = 0; i < len / 1024; i++) {
				ws->ws_http_output_body(sock, data + i * 1024, 1024);
			}
			if (len % 1024 > 0) {
				ws->ws_http_output_body(sock, data + i * 1024, len % 1024);
			}
#endif

		return 0;
	}

	/* body image */
	if (url[0] == 'B') {
		mapfile_t mt;
		const char *file = m_wiki_config->wc_get_body_image_path();
		if (q_mmap(file, &mt)) {
			ws->ws_http_output_head(sock, 200, "image/png", (int)mt.size);
			ws->ws_http_output_body(sock, (char *)mt.start, mt.size);
			q_munmap(&mt);
		}

		return 0;
	}

	/* image */
	if (url[0] == 'I') {
		char *p = (char *)url + 2;
		int size, one_block;
		WikiImage *wiki_image = CURR_WIKI(image);

		if (wiki_image == NULL)
			goto not_found;

		url_convert(p);

		pthread_mutex_lock(&m_mutex);

		if (wiki_image->we_reset(idx, p, &size) == 0) {
#if 0	
			if (strncasecmp(url + strlen(url) - 4, ".svg", 4) == 0)
				ws->ws_http_output_head(sock, 200, "image/svg+xml", size);
			else
#endif
				ws->ws_http_output_head(sock, 200, "image/png", size);

			while (wiki_image->we_read_next(idx, data, &one_block)) {
				ws->ws_http_output_body(sock, data, one_block);
			}
		} else {
			ws->ws_http_output_head(sock, 404, "image/png", 0);
		}
		pthread_mutex_unlock(&m_mutex);

		return 0;
	}

	/* math */
	if (url[0] == 'M') {
		WikiMath *wiki_math = CURR_WIKI(math);
		if (wiki_math != NULL && wiki_math->wm_find(url, data, &len, 1)) {
			ws->ws_http_output_head(sock, 200, "image/png", len);
			ws->ws_http_output_body(sock, data, len);
		} else 
			ws->ws_http_output_head(sock, 404, "image/png", 0);

		return 0;
	}

not_found:

	ws->ws_http_output_head(sock, 404, "text/html", 0);

	return 0;
}

int WikiManage::wiki_conv_key(WikiIndex *wiki_index, const char *start, const char *key, int key_len, char *buf)
{
	int len, len2, found;
	char tmp[KEY_SPLIT_LEN][MAX_KEY_LEN];
	char tmp2[KEY_SPLIT_LEN][MAX_KEY_LEN];
	int size = 0;
	char space[4];

	space[1] = 0;

	memset(tmp, 0, sizeof(tmp));
	memset(tmp2, 0, sizeof(tmp2));

	wiki_index->wi_split_key(key, key_len, tmp, &len);
	wiki_index->wi_split_key(start, strlen(start), tmp2, &len2);

	for (int i = 0; i < len; i++) {
		found = 0;
		for (int j = 0; j < len2; j++) {
			if (my_strcmp(tmp[i], tmp2[j], 0) == 0) {
				found = 1;
				break;
			}
		}
		space[0] = (tmp[i][0] & 0x80) ? 0 : ' ';

		if (found == 0) {
			size += sprintf(buf + size, "%s%s", tmp[i], space);
		} else {
			size += sprintf(buf + size, "<font color=red>%s</font>%s", tmp[i], space);
		}
	}
	buf[size] = 0;

	return 0;
}

int _match_title_cmp(const void *a, const void *b)
{
	wiki_title_t *pa = (wiki_title_t *)a;
	wiki_title_t *pb = (wiki_title_t *)b;

	return strcmp(pa->title, pb->title);
}

int WikiManage::wiki_match(const char *start, wiki_title_t **buf, int *total)
{
	m_match_title_total = 0;
	m_match_title_flag = 0;
	*total = 0;

	if (m_init_flag == 0)
		return 0;

	/* TODO */
	strncpy(m_curr_match_key, start, sizeof(m_curr_match_key) - 1);

	int len, flag;
	struct one_lang *which;
	WikiIndex *wiki_index;
	sort_idx_t sort_idx[MAX_FIND_RECORD + 1];

	char key[512], tmp[2048];
	char *title;

	for (int i = 0; i < m_select_lang_total; i++) {
		if ((which = wiki_get_lang_addr(m_select_lang[i])) == NULL) {
			wiki_init_one_lang(m_select_lang[i], 0);
			if ((which = wiki_get_lang_addr(m_select_lang[i])) == NULL) {
				return -1;
			}
		}
		wiki_index = which->index;

		flag = 0;
		if (wiki_index->wi_find(start, WK_LIKE_FIND, -1, sort_idx, &len) == -1) {
			flag = 1;
			if (wiki_index->wi_find(start, WK_MATCH_FIND, -1, sort_idx, &len) == -1) {
				continue;
			}
		}
		m_match_title_flag++;

		for (int j = 0; j < len; j++) {
			sort_idx_t *p = &sort_idx[j];
			wiki_index->wi_get_key(p, key);

			if (flag) {
				wiki_conv_key(wiki_index, start, key, (int)p->key_len, tmp);
				title = tmp;
			} else
				title = key;

			if (m_match_title_total >= MAX_MATCH_TITLE_TOTAL)
				goto out;

			wiki_title_t *wt = &m_match_title[m_match_title_total++];

			strncpy(wt->title, title, sizeof(wt->title) - 1);
			memcpy(&wt->idx, p, sizeof(wt->idx));
			wt->which = which;
		}
	}

out:

	if (wiki_get_mutil_lang_list_mode()) {
		qsort(m_match_title, m_match_title_total, sizeof(wiki_title_t), _match_title_cmp);
	}

	if (m_match_title_total >= 256)
		m_match_title_total = 256;

	*buf = m_match_title;
	*total = m_match_title_total;

	return m_match_title_total;
}

int WikiManage::wiki_match_total()
{
	return m_match_title_total;
}

int WikiManage::wiki_match_lang(wiki_title_t **buf, int *total, int *flag)
{
	*buf = m_match_title;
	*total = m_match_title_total;
	*flag = m_select_lang_total;

	return m_match_title_total;
}

#include "wiki-httpd.h"

#define MAX_SPLIT_PAGE 16

int WikiManage::wiki_split_content(int len)
{
	int total = len / (100*1024);

	if (total >= MAX_SPLIT_PAGE)
		total = MAX_SPLIT_PAGE - 1;

	total++;

	if (m_wiki_socket->ws_get_server_status() == 0)
		total = 1;

	int pos[MAX_SPLIT_PAGE+1];

	memset(pos, 0, sizeof(pos));

	pos[0] = 0;

	int one_size = len / total;
	char *t = m_curr_content;

	for (int i = 1; i < total; i++) {
		pos[i] = one_size * i;
		for (; t[pos[i]] != '\n'; pos[i]++);
	}
	pos[total] = len;

	for (int i = 0; i < total; i++) {
		m_split_pos[i].start = pos[i] == 0 ? 0 : (pos[i] + 1);
		m_split_pos[i].len = pos[i + 1] - pos[i];
	}

	return total;
}

#define TRANS_FLUSH() \
	do { \
		if (last_len > 0) { \
			last[last_len] = 0; \
			to[pos++] = '<'; \
			to[pos++] = 'k'; \
			to[pos++] = '>'; \
			memcpy(to + pos, last, last_len); \
			pos += last_len; \
			to[pos++] = '<'; \
			to[pos++] = '/'; \
			to[pos++] = 'k'; \
			to[pos++] = '>'; \
			last_len = 0; \
		} \
	} while (0)


int WikiManage::wiki_translate_format(const char *buf, int len, char *to)
{
	char ch, last[1024];
	int last_len = 0;
	int pos = 0;

	for (int i = 0; i < len; i++) {
		if (buf[i] == '<') {
			TRANS_FLUSH();
			for (; i < len; i++) {
				to[pos++] = buf[i];
				if (buf[i] == '>') {
					TRANS_FLUSH();
					break;
				}
			}
		} else {
			ch = buf[i];
			/* "<>,.!?&+:()[]{}\"\r\t\n" */
			if (ch == ' ' || ch == ',' || ch == '.' || ch == '!' || ch == '?' || ch == '&'
					|| ch == '+' || ch == ':' || ch == '(' || ch == ')' || ch == '"' || ch == '\''
					|| ch == '\r' || ch == '\t' || ch == '\n') {
				TRANS_FLUSH();
				to[pos++] = ch;
			} else {
				if (last_len >= (int)sizeof(last) - 1)
					TRANS_FLUSH();
				last[last_len++] = ch;
			}
		}
	}

	TRANS_FLUSH();

	return pos;
}

int WikiManage::wiki_read_data(const sort_idx_t *p, char **buf, const char *title)
{
	char tmp[256];
	const char *key  = tmp;
	int n, pos, height;

	WikiData *wiki_data = CURR_WIKI(data);
	WikiIndex *wiki_index = CURR_WIKI(index);

	INIT_CURR_TITLE();

	if ((n = wiki_data->wd_sys_read((int)p->data_file_idx,
					p->data_pos, (int)p->data_len, m_curr_content, 2*1024*1024)) > 0) {

#if 0
		n = m_wiki_zh->wz_convert_2hans(m_curr_content, n, m_curr_page);
		memcpy(m_curr_content, m_curr_page, n);
		m_curr_content[n] = 0;
#endif

		m_curr_content[n] = 0;

		if (wiki_get_need_translate()) {
			n = wiki_translate_format(m_curr_content, n, m_curr_page);
			memcpy(m_curr_content, m_curr_page, n);
			m_curr_content[n] = 0;
		}

		int split_total = wiki_split_content(n);

		if (title == NULL) {
			wiki_index->wi_get_key(p, tmp);
		} else {
			key = title;
		}
		SET_CURR_TITLE(key);

		wiki_find_key_pos(&pos, &height);

		int len;

		char bg[16], fg[16], link[16], bg_str[64];
		int font_size;

		m_wiki_config->wc_get_config(bg, fg, link, &font_size);

		sprintf(bg_str, "background:%s;\n", bg);

		/* TODO WIKI_START_HTML */
		len = sprintf(m_curr_page, WIKI_START_HTML,
				m_wiki_config->wc_get_body_image_flag() ? "" : bg_str ,
				fg, font_size, link, 
				m_wiki_socket->ws_get_bind_port(), pos, height, split_total - 1,
				m_wiki_config->wc_get_body_image_flag() ? " background=\"B.body.png\"" : "");

		memcpy(m_curr_page + len, m_curr_content + m_split_pos[0].start, m_split_pos[0].len);
		len += m_split_pos[0].len;

		len += sprintf(m_curr_page + len, WIKI_HTTP_END_HTML);

		*buf = m_curr_page;

		return len;
	}

	return 0;
}

int WikiManage::wiki_not_found(char **buf, int *size, const char *str)
{
	char bg[16], fg[16], link[16], bg_str[64];
	int font_size;

	m_wiki_config->wc_get_config(bg, fg, link, &font_size);
	sprintf(bg_str, "background:%s;\n", bg);

	m_buf_len = sprintf(m_buf, WIKI_START_HTML,
			m_wiki_config->wc_get_body_image_flag() ? "" : bg_str,
			fg, font_size, link,
			m_wiki_socket->ws_get_bind_port(), 0, 0, 1,
			m_wiki_config->wc_get_body_image_flag() ? " background=\"B.body.png\"" : "");

	m_buf_len += sprintf(m_buf + m_buf_len, "%s%s", str, WIKI_HTTP_END_HTML);

	m_buf[m_buf_len] = 0;

	*buf = m_buf;
	*size = m_buf_len;

	return m_buf_len;
}

int WikiManage::wiki_view_index(int idx, char **buf, int *size)
{
	int n;
	char tmp[256];
	wiki_title_t *p;
	WikiIndex *wiki_index;

	if (idx >= m_match_title_total) {
		*buf = m_buf;
		*size = wiki_not_found(buf, size, NOT_FOUND);
		return 0;
	}

	p = &m_match_title[idx];
	wiki_index = p->which->index;
	m_curr_lang = p->which;

	wiki_index->wi_get_key(&p->idx, tmp);

	if ((n = wiki_read_data(&p->idx, buf, tmp)) > 0) {
		wiki_push_back(STATUS_MATCH_KEY, m_curr_match_key, 0, NULL);
		wiki_push_back(STATUS_CONTENT, wiki_curr_title(), n, &p->idx);
		*size = n;
		return n;
	}
	
	*buf = m_buf;
	*size = wiki_not_found(buf, size, "Data file error.");

	return 0;
}

int WikiManage::wiki_add_key_pos(int pos, int height, int screen_height)
{
	char title[256];
	char *p = title;
	int len;

	if (m_curr_title[0] == 0)
		return 0;

#ifdef USE_ZH
	if (strcmp(m_wiki_config->wc_get_lang(), "zh") == 0) {
		m_wiki_zh->wz_convert_2hans(m_curr_title, strlen(m_curr_title), p);
		len = strlen(p);
	} else
#endif
	{
		len = strlen(m_curr_title);
		p = m_curr_title;
	}

	return m_wiki_config->wc_add_key_pos(p, len, pos, height, screen_height);
}

int WikiManage::wiki_find_key_pos(int *pos, int *height)
{
	char title[256];
	char *p = title;
	int len;

	if (m_curr_title[0] == 0) {
		*pos = 0;
		*height = 1;
		return 0;
	}

#ifdef USE_ZH
	if (strcmp(m_wiki_config->wc_get_lang(), "zh") == 0) {
		m_wiki_zh->wz_convert_2hans(m_curr_title, strlen(m_curr_title), p);
		len = strlen(p);
	} else
#endif
	{
		len = strlen(m_curr_title);
		p = m_curr_title;
	}

	*pos = m_wiki_config->wc_find_key_pos(p, len, height);

	return *pos;
}

int WikiManage::wiki_add_dir(const char *dir)
{
	return m_wiki_config->wc_add_dir(dir);
}

int WikiManage::wiki_get_fontsize()
{
	return m_wiki_config->wc_get_fontsize();
}

/*
 * fontsize = -1 or 1
 * return -1: can't set font size
 */
int WikiManage::wiki_set_fontsize(int fontsize)
{
	int n = 0;

	if (m_wiki_config) {
		n = m_wiki_config->wc_get_fontsize();
		if (n <= 0)
			n = 13;
		if (n > 3 || fontsize > 0) {
			n += fontsize;
			m_wiki_config->wc_set_fontsize(n);
			return n;
		}
	}

	return n;
}

int WikiManage::wiki_set_select_lang(int *idx, int len)
{
	int total = 0;
	char tmp[MAX_SELECT_LANG_TOTAL][24];

	memset(tmp, 0, sizeof(tmp));

	for (int i = 0; i < m_file_st_total; i++) {
		if (i < len && idx[i] == 1) {
			strcpy(tmp[total++], m_file_st[i].lang);
		}
	}

	if (total == 0)
		return 0;

	memcpy(m_select_lang, tmp, sizeof(m_select_lang));
	m_select_lang_total = total;

	m_wiki_config->wc_set_default_lang(tmp, total);

	return total;
}

int WikiManage::wiki_select_lang(char lang[MAX_SELECT_LANG_TOTAL][24], int *total)
{
	*total = m_select_lang_total;
	memcpy(lang, m_select_lang, sizeof(m_select_lang));

	return 0;
}

char *WikiManage::wiki_about()
{
	int size;
	char *buf, tmp[4096];

	sprintf(tmp, _("FW_ABOUT_MSG"), _VERSION, __DATE__);
	wiki_not_found(&buf, &size, tmp);
	buf[size] = 0;

	return buf;
}

int WikiManage::wiki_get_hide_menu_flag()
{
	return m_wiki_config->wc_get_hide_menu_flag();
}

int WikiManage::wiki_set_hide_menu_flag()
{
	return m_wiki_config->wc_set_hide_menu_flag();
}

int WikiManage::wiki_get_color_mode()
{
	return m_wiki_config->wc_get_color_mode();
}

int WikiManage::wiki_set_color_mode(int idx)
{
	return m_wiki_config->wc_set_color_mode(idx);
}

char *WikiManage::wiki_curr_title()
{
	return m_curr_title;
}

int WikiManage::wiki_href(int href_idx, char **buf, int *size, const char *title)
{
	int n, total;
	sort_idx_t idx[MAX_FIND_RECORD + 1];
	WikiIndex *wiki_index = CURR_WIKI(index);

	if (wiki_index->wi_find(NULL, WK_INDEX_FIND, href_idx, idx, &total) == -1) {
		wiki_not_found(buf, size, _("WIKI_HREF_NOTFOUND")); /* TODO */
		return -1;
	}

	if ((n = wiki_read_data(&idx[0], buf, title)) > 0) {
		wiki_push_back(STATUS_CONTENT, wiki_curr_title(), n, &idx[0]);
		*size = n;
		return 0;
	}

	wiki_not_found(buf, size, NOT_FOUND);

	return -1;
}

int WikiManage::wiki_load_one_page_done()
{
	// TODO
	//strcpy(m_curr_lang, m_all_lang[m_curr_lang_idx].lang);

	return 0;
}

int WikiManage::wiki_find_key(const char *key, char **buf, int *size)
{
	int total, n;
	sort_idx_t idx[MAX_FIND_RECORD + 1];
	WikiIndex *wiki_index;

	if (m_init_flag == 0)
		return -1;
	
	wiki_index = CURR_WIKI(index);

	if (wiki_index->wi_find(key, WK_KEY_FIND, -1, idx, &total) == -1)
		return -1;

	if ((n = wiki_read_data(&idx[0], buf, key)) > 0) {
		wiki_push_back(STATUS_CONTENT, wiki_curr_title(), n, &idx[0]);
		*size = n;
		return 0;
	}

	return -1;
}

int WikiManage::wiki_get_title_from_url(char idx[16][256], char title[16][256], const char *buf)
{
	char *p, tmp[2048];
	int total;
	split_t sp;

	memset(idx, 0, sizeof(idx));
	memset(title, 0, sizeof(title));

	ch2hex(buf, (unsigned char *)tmp);

	if ((p = strchr(tmp, '#')) == NULL)
		return 0;

	*p++ = 0;
	
	total = split(',', tmp, sp);
	
	for (int i = 0; i < total; i++) {
		strncpy(idx[i], sp[i], sizeof(idx[i]) - 1);
	}

	if (split(',', p, sp) != total) {
		return 0;
	}

	for (int i = 0; i < total; i++) {
		strncpy(title[i], sp[i], sizeof(title[i]) - 1);
	}

	return total;
}

int WikiManage::wiki_parse_url_local(const char *url, char *flag, char *ret_title, char **data)
{
	char *ret;
	int total, ret_size;
	char idx[16][256], title[16][256];
	const char *p = strrchr(url, '/');

	if ((total = wiki_get_title_from_url(idx, title, p + 1)) <= 0)
		return 0;

	for (int i = 0; i < total; i++) {
		if (wiki_href(atoi(idx[i]), &ret, &ret_size, title[i]) == 0) {
			strcpy(flag, "1");
			strcpy(ret_title, title[i]);
			*data = ret;
#ifdef FW_DEBUG
			LOG("title=%s.\n", title[i]);
#endif
			return 1;
		}
	}

	for (int i = 0; i < total; i++) {
		char _tmp[256];
		strcpy(_tmp, title[i]);
		url_convert(_tmp);
		if (wiki_find_key(_tmp, &ret, &ret_size) == 0) {
			strcpy(flag, "1");
			strcpy(ret_title, _tmp);
			*data = ret;
			return 1;
		}
	}

	strcpy(flag, "1");
	strcpy(m_buf, "Not Found.\n");
	*data = m_buf;

	return 0;
}

/*
 * parse "lang:dictionary:title"
 */
int WikiManage::wiki_parse_url_lang(const char *url, char *flag, char *title, char **data)
{
	int ret_size;
	char *p, tmp[128];
	struct one_lang *which;

	strcpy(tmp, url);
	url_convert(tmp);

	if ((p = strchr(tmp, ':')) == NULL)
		return -1;

	*p++ = 0;

	if ((which = wiki_get_lang_addr(tmp)) == NULL) {
		wiki_init_one_lang(tmp, 0);
		if ((which = wiki_get_lang_addr(tmp)) == NULL)
			return -1;
	}

	m_curr_lang = which;

	if (wiki_find_key(p, data, &ret_size) == 0) {
		strcpy(title, p);
		strcpy(flag, "1");
		return 0;
	}

	return -1;
}

int WikiManage::wiki_parse_url(const char *url, char *flag, char *title, char **data)
{
	char *ret;
	int ret_size;

	if (strcasecmp(url, "__curr__") == 0) {
		strcpy(title, wiki_curr_title());
		if (wiki_find_key(title, &ret, &ret_size) == 0) {
			strcpy(flag, "1");
			*data = ret;
		} else {
			strcpy(flag, "0");
			strcpy(m_buf, "Not Found.\n");
			*data = m_buf;
		}
	} else if (strncasecmp(url, "http", 4) == 0) {
		if (strncmp(url + 4, "://127.0.0.1", 11) == 0) {
			wiki_parse_url_local(url, flag, title, data);
		} else {
			strcpy(flag, "2");
			strcpy(title, url);
		}
	} else if (strncmp(url, "lang:", 5) == 0) {
		if (wiki_parse_url_lang(url + 5, flag, title, data) == -1) {
			strcpy(flag, "0");
			strcpy(m_buf, "Not Found.\n");
			*data = m_buf;
		}
	}

	return 0;
}

int WikiManage::wiki_random_history(char **buf, int *size)
{
	struct one_lang *which;

	history_key_t key;
	history_value_t value;

	if (!m_wiki_history->wh_random(&key, &value))
		return wiki_not_found(buf, size, _("FW_RANDOM_RETRY"));

	if ((which = wiki_get_lang_addr(key.lang)) == NULL) {
		wiki_init_one_lang(key.lang, 0);
		if ((which = wiki_get_lang_addr(key.lang)) == NULL) {
			return wiki_not_found(buf, size, "Not found");
		}
	}
	m_curr_lang = which;

	return wiki_find_key(key.title, buf, size);
}

int WikiManage::wiki_random_favorite(char **buf, int *size)
{
	struct one_lang *which;

	struct fav_key key;
	struct fav_value value;

	if (!m_wiki_favorite->wf_random(&key, &value))
		return wiki_not_found(buf, size, _("FW_RANDOM_RETRY"));

	if ((which = wiki_get_lang_addr(key.lang)) == NULL) {
		wiki_init_one_lang(key.lang, 0);
		if ((which = wiki_get_lang_addr(key.lang)) == NULL) {
			return wiki_not_found(buf, size, "Not found");
		}
	}
	m_curr_lang = which;

	return wiki_find_key(key.title, buf, size);
}

int WikiManage::wiki_random_select_lang(char **buf, int *size)
{
	sort_idx_t idx;
	int n = rand() % m_select_lang_total;
	struct one_lang *which;

	if (m_select_lang_total <= 0) /* TODO */
		return wiki_not_found(buf, size, "I'm Blank");


	if ((which = wiki_get_lang_addr(m_select_lang[n])) == NULL) {
		wiki_init_one_lang(m_select_lang[n], 0);
		if ((which = wiki_get_lang_addr(m_select_lang[n])) == NULL) {
			return wiki_not_found(buf, size, "Not found");
		}
	}

	m_curr_lang = which;

	for (int i = 0; i < 100; i++) {
		which->index->wi_random_key(&idx);
		if ((n = wiki_read_data(&idx, buf, NULL)) > 0) {
			wiki_push_back(STATUS_CONTENT, wiki_curr_title(), n, &idx);
			*size = n;
			return n;
		}
	}

	return -1;
}

int WikiManage::wiki_random_all_lang(char **buf, int *size)
{

	int total = 0;
	struct lang_list lang[128];
	struct one_lang *which;
	sort_idx_t idx;

	memset(lang, 0, sizeof(lang));
	wiki_get_lang_list(lang, &total);

	if (total <= 0)
		return -1;

	int n = rand() % total;

	if ((which = wiki_get_lang_addr(lang[n].lang)) == NULL) {
		wiki_init_one_lang(lang[n].lang, 0);
		if ((which = wiki_get_lang_addr(lang[n].lang)) == NULL) {
			return wiki_not_found(buf, size, "Not found");
		}
	}

	m_curr_lang = which;

	for (int i = 0; i < 100; i++) {
		which->index->wi_random_key(&idx);
		if ((n = wiki_read_data(&idx, buf, NULL)) > 0) {
			wiki_push_back(STATUS_CONTENT, wiki_curr_title(), n, &idx);
			*size = n;
			return n;
		}
	}

	return -1;
}

int WikiManage::wiki_random_page(char **buf, int *size)
{
	int n = -1, flag = wiki_get_random_flag();

	switch (flag) {
		case RANDOM_HISTORY:
			n = wiki_random_history(buf, size);
			break;

		case RANDOM_FAVORITE:
			n = wiki_random_favorite(buf, size);
			break;

		case RANDOM_SELECT_LANG:
			n = wiki_random_select_lang(buf, size);
			break;

		case RANDOM_ALL_LANG:
			n = wiki_random_all_lang(buf, size);
			break;
		default:
			break;
	};

	if (n >= 0)
		return n;

	return wiki_not_found(buf, size, "I'm Blank.");
}

int WikiManage::wiki_page_last_read(char **buf, int *size)
{
	struct one_lang *which;
	history_key_t key;
	history_value_t value;

	m_wiki_history->wh_get_last_record(&key, &value);

	wiki_init_one_lang(key.lang, 0);
	if ((which = wiki_get_lang_addr(key.lang)) == NULL)
		return -1;

	m_curr_lang = which;

	int n, len;
	sort_idx_t sort_idx[MAX_FIND_RECORD + 1];

	if (which->index->wi_find(key.title, WK_LIKE_FIND, -1, sort_idx, &len) == -1)
		return -1;

	if ((n = wiki_read_data(&sort_idx[0], buf, key.title)) > 0) {
		*size = n;
		return n;
	}

	return -1;
}

int WikiManage::wiki_curr_date_home_page(char **buf, int *size)
{
	int pos = 0, total = 0;
	char *lang, date[128], tmp[4096], save_date[128];
	struct one_lang *which;

	pos += sprintf(tmp, "<table cellspacing=0.5 width=100%% border=1>\n"
					"<tr><td width=30%%>%s</td><td width=70%%>%s</td></tr>\n",
					_("FW_HP_DICT"), _("FW_HP_CURR_DATE"));

	for (int i = 0; i < m_select_lang_total; i++) {
		lang = m_select_lang[i];
		get_month_day_from_lang(date, lang);

		if (date[0] == 0)
			continue;

		if ((which = wiki_get_lang_addr(lang)) == NULL) {
			wiki_init_one_lang(lang, 0);
			if ((which = wiki_get_lang_addr(lang)) == NULL) {
				continue;
			}
		}

		total++;
		strcpy(save_date, date);
		m_curr_lang = which;
		pos += snprintf(tmp + pos, sizeof(tmp) - pos,
					"<tr><td width=30%%>%s</td><td width=70%%><a href='lang:%s:%s'>%s</a></td></tr>\n",
					lang, lang, date, date);
	}

	if (total == 0)
		strcpy(tmp, _("CANNT_FIND_CURR_DATE"));
	else if (total == 1)
		return wiki_find_key(save_date, buf, size);
	else
		pos += snprintf(tmp + pos, sizeof(tmp) - pos, "%s", "</table>\n");

	return wiki_not_found(buf, size, tmp);
}

int WikiManage::wiki_index_msg(char **buf, int *size)
{
	int n;
	int flag = m_wiki_config->wc_get_home_page_flag();

	switch (flag) {
		case HOME_PAGE_BLANK:
			char tmp[1024];
			sprintf(tmp, _("WELCOME"), _VERSION, __DATE__);
			return wiki_not_found(buf, size, tmp);
			break;

		case HOME_PAGE_LAST_READ:
			if ((n = wiki_page_last_read(buf, size)) != -1) 
				return n;
			break;

		case HOME_PAGE_CURR_DATE:
			if ((n = wiki_curr_date_home_page(buf, size)) != -1)
				return n;
			break;

		default:
			break;
	}

	return wiki_not_found(buf, size, NOT_FOUND);
}

int WikiManage::wiki_push_back(int status, const char *title, int size, const sort_idx_t *idx)
{

	if (m_curr_lang != NULL)
		m_wiki_history->wh_add_cache(status, title, m_curr_lang->lang, size, idx);

	return 0;
}

int WikiManage::wiki_curr(char **buf, int *size)
{
	int n;
	const cache_t *p = m_wiki_history->wh_curr_cache();

	if (p == NULL)
		return wiki_index_msg(buf, size);

	if ((n = wiki_read_data(&p->idx, buf, NULL)) > 0) {
		*size = n;
		return n;
	}

	return 0;
}

int WikiManage::wiki_back(char *status, char **buf, int *size)
{
	const cache_t *p = m_wiki_history->wh_back_cache();

	if (p == NULL)
		return 0;

	if (p->flag == STATUS_CONTENT) {
		strcpy(status, "1");
		return wiki_curr(buf, size);
	}

	strcpy(status, "2");
	*buf = (char *)p->key.title;

	return 1;
}

int WikiManage::wiki_forward(char *status, char **buf, int *size)
{
	const cache_t *p = m_wiki_history->wh_forward_cache();

	if (p == NULL)
		return 0;

	if (p->flag == STATUS_CONTENT) {
		strcpy(status, "1");
		return wiki_curr(buf, size);
	}

	strcpy(status, "2");
	*buf = (char *)p->key.title;

	return 1;
}

int _cmp_tmp_history(const void *a, const void *b)
{
	const struct tmp_history *x, *y;

	x = (const struct tmp_history *)a;
	y = (const struct tmp_history *)b;

	return y->read_time - x->read_time;
}

int WikiManage::wiki_history_begin()
{
	struct tmp_history *p;
	history_key_t *key;
	history_value_t value;

	m_history_total = 0;

	m_wiki_history->wh_history_begin();

	while (m_wiki_history->wh_history_next(&key, &value) && m_history_total < MAX_TMP_HISTORY_TOTAL) {
		p = &m_history[m_history_total++];
		p->data = key;
		p->read_time = value.time;
		if (m_history_total >= 256)
			break;
	}

	if (m_history_total > 0)
		qsort(m_history, m_history_total, sizeof(struct tmp_history), _cmp_tmp_history);

	return m_history_total;
}

int WikiManage::wiki_history_next(int idx, char *title)
{
	struct tmp_history *p = &m_history[idx];

	strcpy(title, p->data->title);

	return 0;
}

int WikiManage::wiki_delete_history(int idx)
{
	struct tmp_history *p = &m_history[idx];

	m_wiki_history->wh_delete_history(p->data->title, p->data->lang);

	return 0;
}

int WikiManage::wiki_clean_history()
{
	m_wiki_history->wh_clean_history();

	return 0;
}

int WikiManage::wiki_view_history(int idx, char **buf, int *size)
{
	int n = 0;
	char tmp[1024];
	struct tmp_history *p = &m_history[idx];

	if (idx < 0 || idx >= m_history_total)
		return 0;

	if (wiki_init_one_lang(p->data->lang, 1) == -1) {
		sprintf(tmp, "Not found dictionary: %s", p->data->lang);
		return wiki_not_found(buf, size, tmp);
	}

	if ((n = wiki_find_key(p->data->title, buf, size)) == -1) {
		sprintf(tmp, "Not found %s in dictionary %s", p->data->title, p->data->lang);
		return wiki_not_found(buf, size, tmp);
	}

	return *size;
}

int WikiManage::wiki_get_full_screen_flag()
{
	return m_wiki_config->wc_get_full_screen_flag();
}

int WikiManage::wiki_switch_full_screen()
{
	return m_wiki_config->wc_switch_full_screen();
}

int WikiManage::wiki_get_list_color(char *list_bg, char *list_fg)
{
	return m_wiki_config->wc_get_list_color(list_bg, list_fg);
}

int WikiManage::wiki_view_favorite(int idx, char **buf, int *size)
{
	int n;
	struct tmp_favorite *p = &m_favorite[idx];

	/* TODO */
	wiki_init_one_lang(p->data->lang, 1);

	if ((n = wiki_find_key(p->data->title, buf, size)) == 0) {
		n = *size;
	}

	return n;
}

int WikiManage::wiki_favorite_rate(const char *title, const char *lang, char *percent)
{
	int height, screen_height;
	int pos = m_wiki_config->wc_find_key_pos(title, strlen(title), &height, &screen_height);

	if (pos == 0) {
		strcpy(percent, "0%");
		return 0;
	}

	if (height < 10 || pos + screen_height >= height) {
		strcpy(percent, "100%");
	} else {
		sprintf(percent, "%.0f%%", (float)(pos + screen_height) * 100.0 / (float)height);
	}

	return 0;
}

int _cmp_tmp_favorite(const void *a, const void *b)
{
	struct tmp_favorite *x, *y;

	x = (struct tmp_favorite *)a;
	y = (struct tmp_favorite *)b;

	return y->value.last_time - x->value.last_time;
}

int WikiManage::wiki_favorite_begin()
{
	struct tmp_favorite *p;
	struct fav_key *key;
	struct fav_value value;

	m_favorite_total = 0;

	m_wiki_favorite->wf_begin();

	while (m_wiki_favorite->wf_next(&key, &value) && m_favorite_total < MAX_TMP_FAVORITE_TOTAL) {
		p = &m_favorite[m_favorite_total++];
		p->data = key;
		memcpy(&p->value, &value, sizeof(struct fav_value));
		if (m_favorite_total >= 256)
			break;
	}

	if (m_favorite_total > 0)
		qsort(m_favorite, m_favorite_total, sizeof(struct tmp_favorite), _cmp_tmp_favorite);

	return m_favorite_total;
}

int WikiManage::wiki_favorite_total()
{
	return m_favorite_total;
}

int WikiManage::wiki_favorite_next(int idx, char *title, char *rate)
{
	struct tmp_favorite *p = &m_favorite[idx];

	strcpy(title, p->data->title);
	wiki_favorite_rate(p->data->title, p->data->lang, rate);

	return 0;
}

int WikiManage::wiki_is_favorite()
{
	if (m_curr_title[0] == 0 || m_curr_lang == NULL)
		return 0;

	return m_wiki_favorite->wf_find(m_curr_title, m_curr_lang->lang);
}

int WikiManage::wiki_add_favorite()
{
	if (m_curr_title[0] && m_curr_lang != NULL) {
		m_wiki_favorite->wf_add(m_curr_title, m_curr_lang->lang, 0);
		return 1;
	}

	return 0;
}

int WikiManage::wiki_delete_favorite()
{
	/* TODO */
	if (m_curr_title[0] && m_curr_lang != NULL) {
		m_wiki_favorite->wf_delete(m_curr_title, m_curr_lang->lang);
	}

	return 0;
}

int WikiManage::wiki_delete_favorite_one(int idx)
{
	struct tmp_favorite *p = &m_favorite[idx];

	m_wiki_favorite->wf_delete(p->data->title, p->data->lang);

	return 0;
}

int WikiManage::wiki_clean_favorite()
{
	m_wiki_favorite->wf_delete_all();

	return 0;
}

int WikiManage::wiki_translate_find(struct one_lang *p, const char *key, char *buf)
{
	sort_idx_t idx[16];
	int total,  len;
	int show_line = wiki_get_translate_show_line();

	if (p == NULL || p->index == NULL || p->data == NULL)
		return 0;

	p->index->wi_find(key, WK_KEY_FIND, -1, idx, &total);

	if (total <= 0)
		return 0;
	
	if ((len = p->data->wd_sys_read(idx[0].data_file_idx, idx[0].data_pos, idx[0].data_len,
					buf, MAX_TRANS_BUF_SIZE)) > 0) {
		buf[len] = 0;

		int count = 0;
		for (char *t = buf; *t; t++) {
			if (*t == '\n')
				count++;
			if (count > show_line) {
				*++t = 0;
				break;
			}
		}
		return 1;
	}

	return 0;
}

int WikiManage::wiki_translate2(struct one_lang *p, const char *key, int len, char *buf)
{
	int flag;
	char tmp[1024];

	strcpy(tmp, key);
	flag = wiki_translate_find(p, tmp, buf);

	if (flag == 0) {
		if (tmp[len - 1] == 'd' && (len > 1 && strncmp(tmp + len - 2, "ed", 2) == 0)) {
			tmp[len - 1] = 0;
			flag = wiki_translate_find(p, tmp, buf);
			if (flag == 0) {
				tmp[len - 2] = 0;
				flag = wiki_translate_find(p, tmp, buf);
			}
		}
	}

	if (flag == 0) {
		if (tmp[len - 1] == 's') {
			tmp[len - 1] = 0;
			flag = wiki_translate_find(p, tmp, buf);
		}
	}

	if (flag == 0) {
		if (tmp[len - 1] == '\'' || tmp[len - 1] == '-') {
			tmp[len - 1] = 0;
			flag = wiki_translate_find(p, tmp, buf);
		}
	}

	if (flag == 0) {
		if (tmp[0] == '\'' || tmp[0] == '-') {
			flag = wiki_translate_find(p, tmp + 1, buf);
		}
	}

	if (flag == 0) {
		if (len > 3 && strncmp(tmp + len - 3, "ing", 3) == 0) {
			tmp[len - 3] = 0;
			len -= 3;
			flag = wiki_translate_find(p, tmp, buf);
			if (flag == 0) {
				tmp[len++] = 'e';
				tmp[len] = 0;
				flag = wiki_translate_find(p, tmp, buf);
			}
		}
	}

	if (flag == 0)
		sprintf(buf, "Not found %s.\n", key);

	return 0;
}

int WikiManage::wiki_translate(const char *key, char **buf)
{
	char lang[32];

	memset(lang, 0, sizeof(lang));
	wiki_get_translate_default(lang);

	return wiki_sys_translate(key, lang, buf);
}

int WikiManage::wiki_sys_translate(const char *key, const char *lang, char **buf)
{
	struct one_lang *p = wiki_get_lang_addr(lang);

	if (p == NULL) {
		wiki_init_one_lang(lang, 0);
		if ((p = wiki_get_lang_addr(lang)) == NULL) {
			sprintf(m_trans_buf, "Not found %s.<br/>\nRead data file error.", key);
			*buf = m_trans_buf;
			return -1;
		}
	}

	char tmp[1024];
	int x = 0;

	for (const char *t = key; *t; t++) {
		if (*t >= 'A' && *t <= 'Z') {
			tmp[x++] = *t + 'a' - 'A';
		} else {
			tmp[x++] = *t;
		}
	}
	tmp[x] = 0;

	*buf = m_trans_buf;

	wiki_translate2(p, tmp, x, m_trans_buf);

	return 0;
}

struct one_lang *WikiManage::wiki_get_lang_addr(const char *lang)
{
	struct one_lang *p;

	for (int i = 0; i < m_all_lang_total; i++) {
		p = &m_all_lang[i];
		if (strcmp(p->lang, lang) == 0 && p->flag == 1)
			return p;
	}

	return NULL;
}

int WikiManage::wiki_get_mutil_lang_list_mode()
{
	return m_wiki_config->wc_get_mutil_lang_list_mode();
}

int WikiManage::wiki_set_mutil_lang_list_mode(int mode)
{
	return m_wiki_config->wc_set_mutil_lang_list_mode(mode);
}

int WikiManage::wiki_get_translate_show_line()
{
	return m_wiki_config->wc_get_translate_show_line();
}

int WikiManage::wiki_set_translate_show_line(int x)
{
	return m_wiki_config->wc_set_translate_show_line(x);
}

int WikiManage::wiki_get_use_language()
{
	return m_wiki_config->wc_get_use_language();
}

int WikiManage::wiki_set_use_language(int idx)
{
	return m_wiki_config->wc_set_use_language(idx);
}

int WikiManage::wiki_set_home_page_flag(int idx)
{
	return m_wiki_config->wc_set_home_page_flag(idx);
}

int WikiManage::wiki_get_home_page_flag()
{
	return m_wiki_config->wc_get_home_page_flag();
}

int WikiManage::wiki_set_need_translate(int idx)
{
	return m_wiki_config->wc_set_need_translate(idx);
}

int WikiManage::wiki_get_need_translate()
{
	return m_wiki_config->wc_get_need_translate();
}

int WikiManage::wiki_get_translate_default(char *lang)
{
	return m_wiki_config->wc_get_translate_default(lang);
}

int WikiManage::wiki_set_translate_default(const char *lang)
{
	return m_wiki_config->wc_set_translate_default(lang);
}

int WikiManage::wiki_scan_sdcard()
{
	m_wiki_config->wc_scan_all();
	wiki_lang_init();

	if (m_select_lang_total > 0)
		m_init_flag = 1;

	return 0;
}

int WikiManage::wiki_set_random_flag(int idx)
{
	return m_wiki_config->wc_set_random_flag(idx);
}

int WikiManage::wiki_get_random_flag()
{
	return m_wiki_config->wc_get_random_flag();
}


int WikiManage::wiki_get_body_image_flag()
{
	return m_wiki_config->wc_get_body_image_flag();
}

int WikiManage::wiki_set_body_image_flag(int flag)
{
	return m_wiki_config->wc_set_body_image_flag(flag);
}

const char *WikiManage::wiki_get_body_image_path()
{
	return m_wiki_config->wc_get_body_image_path();
}

int WikiManage::wiki_set_body_image_path(const char *path)
{
	return m_wiki_config->wc_set_body_image_path(path);
}

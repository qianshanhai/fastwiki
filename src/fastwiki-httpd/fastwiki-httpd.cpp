/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "q_util.h"

#include "wiki_data.h"
#include "wiki_index.h"
#include "wiki_full_index.h"

#include "wiki_config.h"
#include "wiki_local.h"
#include "wiki_socket.h"
#include "wiki_math.h"
#include "wiki_image.h"

#include "fastwiki-httpd.h"

#define MAX_PTHREAD_TOTAL 10
#define MAX_PAGE_LEN (5*1024*1024)

char *m_math_data[MAX_PTHREAD_TOTAL];
char *m_buf_data[MAX_PTHREAD_TOTAL];
char *m_tmp_data[MAX_PTHREAD_TOTAL];

char start_html[4096];
char end_html[4096];
int start_html_len = 0;
int end_html_len = 0;

static WikiConfig *m_wiki_config = NULL;

static WikiIndex *m_wiki_index = NULL;
static WikiData *m_wiki_data = NULL;
static WikiMath *m_wiki_math = NULL;
static WikiImage *m_wiki_image = NULL;
static WikiFullIndex *m_wiki_full_text = NULL;

static WikiSocket *m_wiki_socket = NULL;

static struct file_st *m_file;
static data_head_t m_data_head;

#define LOG printf

#include "wiki_common.h"

static void my_wait()
{
	printf("Init done: http://127.0.0.1:%d\n", m_wiki_socket->ws_get_bind_port());

#ifdef DEBUG
	printf("-- This is debug version --\n");
#endif

	for (;;) {
		q_sleep(1);
	}
}

static int output_one_page(int sock, WikiSocket *ws, const char *data, int n, const char *K)
{
	char tmp[4096];
	int tmp_len = 0;
	int size;
	
	tmp_len = sprintf(tmp, end_html, K);
	size = start_html_len + tmp_len + n;

	ws->ws_http_output_head(sock, 200, "text/html", size);
	ws->ws_http_output_body(sock, start_html, start_html_len);
	ws->ws_http_output_body(sock, data, n);
	ws->ws_http_output_body(sock, tmp, tmp_len);

	return 1;
}

int check_digit(const char *p)
{
	for (; *p; p++) {
		if (!((*p >= '0' && *p <= '9') || *p == ','))
			return 0;
	}

	return 1;
}

/*
 * copy from wiki.cpp
 */
int wiki_conv_key(const char *start, const char *key, int key_len, char *buf)
{
	int len, len2, found;
	char tmp[KEY_SPLIT_LEN][MAX_KEY_LEN];
	char tmp2[KEY_SPLIT_LEN][MAX_KEY_LEN];
	int size = 0;
	char space[4];

	space[1] = 0;

	memset(tmp, 0, sizeof(tmp));
	memset(tmp2, 0, sizeof(tmp2));

	m_wiki_index->wi_split_key(key, key_len, tmp, &len);
	m_wiki_index->wi_split_key(start, strlen(start), tmp2, &len2);

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

char *convert_nohtml(const char *buf, int n, char *tmp, int *tmp_len)
{
	int len = 0;

	if (m_data_head.complete_flag == 0) {
		*tmp_len = n;
		return (char *)buf;
	}

	for (int i = 0; i < n; i++) {
		if (buf[i] == '<') {
			memcpy(tmp + len, "&lt;", 4);
			len += 4;
		} else if (buf[i] == '>') {
			memcpy(tmp + len, "&gt;", 4);
			len += 4;
		} else if (buf[i] == '\n') {
			memcpy(tmp + len, "<br/>\n", 6);
			len += 6;
		} else
			tmp[len++] = buf[i];
	}

	*tmp_len = len;

	return tmp;
}

#define _FH_IS_CHAPTER_END(p) (0[p] == '<' && (1[p] == 'h' || 1[p] == 'H') \
		&& (2[p] == 'r' || 2[p] == 'R') && 3[p] == ' ' && (4[p] == 'a' || 4[p] == 'A') \
		&& (5[p] == 'l' || 5[p] == 'L') && (6[p] == 'i' || 6[p] == 'I') && (7[p] == 'g' || 7[p] == 'G') \
		&& (8[p] == 'n' || 8[p] == 'N') && 9[p] == '=' && (10[p] == 'l' || 10[p] == 'L') \
		&& (11[p] == 'e' || 11[p] == 'E') && (12[p] == 'f' || 12[p] == 'F') && (13[p] == 't' || 13[p] == 'T') \
		&& 14[p] == ' ' && (15[p] == 'w' || 15[p] == 'W') && (16[p] == 'i' || 16[p] == 'I') \
		&& (17[p] == 'd' || 17[p] == 'D') && (18[p] == 't' || 18[p] == 'T') && (19[p] == 'h' || 19[p] == 'H') \
		&& 20[p] == '=' && 21[p] == '3' && 22[p] == '5' && 23[p] == '%' && 24[p] == '>') 


#define _FW_URL_FUNC_ARGV WikiSocket *ws, HttpParse *http, int sock, int pthread_idx

static int my_find_full_text(_FW_URL_FUNC_ARGV, const char *key)
{
	int n, total;
	char *buf_data = m_buf_data[pthread_idx];
	char *tmp_data = m_tmp_data[pthread_idx];
	char title[128];
	int page_idx[16];

	LOG("key:%s.\n", key);
	fflush(stdout);

	total = m_wiki_full_text->wfi_find(key, page_idx, sizeof(page_idx) / sizeof(int));

	if (total <= 0) {
		n = sprintf(buf_data, "Not found: %s\n", key);
		return output_one_page(sock, ws, buf_data, n, key);
	}

	int pos = 0, len;
	sort_idx_t idx;

	pos += sprintf(tmp_data + pos, "about total: %d <br/><br/>\n", total);
	pos += sprintf(tmp_data + pos, "<table border=1>\n");

	for (int i = 0; i < total; i++) {
		if (m_wiki_index->wi_find(NULL, WK_INDEX_FIND, page_idx[i], &idx, &len) == -1)
			continue;

		m_wiki_index->wi_get_key(&idx, title);
		if ((len = m_wiki_data->wd_sys_read(idx.data_file_idx, idx.data_pos, idx.data_len, buf_data, MAX_PAGE_LEN)) <=  0)
			continue;

		int found = 0;
		int j = 0;
		for (j = 0; j < len && j < 4096; j++) {
			if (_FH_IS_CHAPTER_END(buf_data + j)) {
				j += 25;
				found = 1;
				break;
			}
		}
		if (found == 0)
			j = 0;
		/*
		   if (len - j >= 512)
		   len = j + 512;
		   */
		buf_data[len] = 0;
		pos += sprintf(tmp_data + pos, "<tr><td>idx:%d, title: %s<br/>\n", page_idx[i], title);
		pos += sprintf(tmp_data + pos, "%s<br/><br/></td</tr>\n", buf_data + j);
	}

	pos += sprintf(tmp_data + pos, "</table>\n");
	output_one_page(sock, ws, tmp_data, pos, key);

	return 0;
}

static int my_find_key(_FW_URL_FUNC_ARGV)
{
	int total, n;
	sort_idx_t idx;
	char *buf_data = m_buf_data[pthread_idx];
	char *tmp_data = m_tmp_data[pthread_idx];
	char *key = http->hp_param("key");
	char *ft = http->hp_param("ft");

	trim(key); /* TODO */

	if (ft[0])
		return my_find_full_text(ws, http, sock, pthread_idx, key);

	if (m_wiki_index->wi_find(key, WK_KEY_FIND, -1, &idx, &total) == 0) {
		if ((n = m_wiki_data->wd_sys_read((int)idx.data_file_idx, idx.data_pos,
						(int)idx.data_len, buf_data, MAX_PAGE_LEN)) > 0) {
			char *data = convert_nohtml(buf_data, n, tmp_data, &n);
			output_one_page(sock, ws, data, n, key);
			return 0;
		}
	}

	return output_one_page(sock, ws, "Not Found", 9, "");
}

static int my_find_image(_FW_URL_FUNC_ARGV)
{
	int size, one_block;
	char *buf_data = m_buf_data[pthread_idx];
	char *file = http->hp_param("file");

#ifndef WIN32
	if (m_wiki_image == NULL) {
		m_wiki_image = new WikiImage();
		m_wiki_image->we_init(m_file->image_file, m_file->image_total);
	}
#endif
	if (m_wiki_image->we_reset(pthread_idx, file, &size) == 0) {
		if (strncasecmp(file + strlen(file) - 4, ".svg", 4) == 0)
			ws->ws_http_output_head(sock, 200, "image/svg+xml", size);
		else
			ws->ws_http_output_head(sock, 200, "image/png", size);

		while (m_wiki_image->we_read_next(pthread_idx, buf_data, &one_block)) {
			ws->ws_http_output_body(sock, buf_data, one_block);
		}
		return 0;
	}

	return 0;
}

static int my_find_math(_FW_URL_FUNC_ARGV)
{
	int len;
	char *file = http->hp_param("file");
	char *buf_data = m_buf_data[pthread_idx];

#ifndef WIN32
	if (m_wiki_math == NULL) {
		m_wiki_math = new WikiMath();
		m_wiki_math->wm_init(m_file->math_file);
	}
#endif
	if (m_wiki_math->wm_find(file, buf_data, &len, 0)) {
		ws->ws_http_output_head(sock, 200, "image/png", len);
		ws->ws_http_output_body(sock, buf_data, len);
		return 0;
	}

	return 0;
}

static int my_find_match(_FW_URL_FUNC_ARGV)
{
	sort_idx_t idx[MAX_FIND_RECORD];
	char *key = http->hp_param("key");

	trim(key); /* TODO */

	int len, total;
	int match_flag = 0;

	if (m_wiki_index->wi_find(key, WK_LIKE_FIND, -1, idx, &total) == -1) {
		match_flag = 1;
		if (m_wiki_index->wi_find(key, WK_MATCH_FIND, -1, idx, &total) == -1) {
			char buf[] = "Not Found\n";
			len = strlen(buf);
			//output_one_page(sock, ws, buf, len, p);
			ws->ws_http_output_head(sock, 200, "text/html", len);
			ws->ws_http_output_body(sock, buf, len);
			return 0;
		}
	}

	char title[1024];
	char buf[4096];
	char tmp[2048];

	len = 0;

	for (int i = 0; i < total && i < 25; i++) {
		sort_idx_t *x = &idx[i];
		m_wiki_index->wi_get_key(x, title);
		if (match_flag) {
			wiki_conv_key(key, title, (int)x->key_len, tmp);
			len += snprintf(buf + len, sizeof(buf) - len,
					"<a href='#%s' onclick=\"get('pos?idx=%d&pos=%d&len=%d');\">%s</a><br/>",
					title, x->data_file_idx, x->data_pos, x->data_len, tmp);
		} else {
			len += snprintf(buf + len, sizeof(buf) - len,
					"<a href='#%s' onclick=\"get('pos?idx=%d&pos=%d&len=%d');\">%s</a><br/>",
					title, x->data_file_idx, x->data_pos, x->data_len, title);
		}
	}

	ws->ws_http_output_head(sock, 200, "text/html", len);
	ws->ws_http_output_body(sock, buf, len);

	return 0;
}

static int my_find_pos(_FW_URL_FUNC_ARGV)
{
	int n, file_idx, pos, len;
	char *buf_data = m_buf_data[pthread_idx];
	char *tmp_data = m_tmp_data[pthread_idx];

	file_idx = atoi(http->hp_param("idx"));
	pos = atoi(http->hp_param("pos"));
	len = atoi(http->hp_param("len"));

	if ((n = m_wiki_data->wd_sys_read(file_idx, (unsigned int)pos, len, buf_data, MAX_PAGE_LEN)) > 0) {
		char *w = convert_nohtml(buf_data, n, tmp_data, &n);
		ws->ws_http_output_head(sock, 200, "text/html", n);
		ws->ws_http_output_body(sock, w, n);
		return 0;
	}
	
	return 0;
}

static int my_find_index(_FW_URL_FUNC_ARGV)
{
	split_t sp;
	sort_idx_t idx;
	int total, n;
	char key[1024] = {0};
	const char *url = http->hp_url();

	char *buf_data = m_buf_data[pthread_idx];
	char *tmp_data = m_tmp_data[pthread_idx];

	split(',', url, sp);

	for (int i = 0; i < split_total(sp); i++) {
		if (m_wiki_index->wi_find(NULL, WK_INDEX_FIND, atoi(sp[i]), &idx, &total) == 0) {
			if ((n = m_wiki_data->wd_sys_read((int)idx.data_file_idx, idx.data_pos,
							(int)idx.data_len, buf_data, MAX_PAGE_LEN)) > 0) {
				m_wiki_index->wi_get_key(&idx, key);
				char *w = convert_nohtml(buf_data, n, tmp_data, &n);
				output_one_page(sock, ws, w, n, key);
				return 0;
			}
		}
	}

	return output_one_page(sock, ws, "Not Found", 9, "");
}

struct url_func {
	const char *url;
	int (*func)(_FW_URL_FUNC_ARGV);
};

struct url_func m_url_func[] = {
	{"search", my_find_key},
	{"match", my_find_match},
	{"i.jpg", my_find_image},
	{"m.jpg", my_find_math},
	{"pos", my_find_pos},
	{NULL, NULL}
};

/*
 *  search ? key=xxx
 *  match  ? key=xxx
 *  i.jpg  ? file=xxx
 *  m.jpg  ? file=xxx
 *  pos    ? idx=xxx, pos=, len
 */
static int my_do_url(void *_class, void *type, void *_http, int sock, int pthread_idx)
{
	const char *url;
	WikiSocket *ws = (WikiSocket *)type;
	HttpParse *http = (HttpParse *)_http;
	char *ft = http->hp_param("ft");

	start_html_len = sprintf(start_html, HTTP_START_HTML, ft[0] ? "checked" : "");
	end_html_len = sprintf(end_html, "%s", HTTP_END_HTML);

	url = http->hp_url();

#ifdef DEBUG
	LOG("url=%s.\n", url);
#endif

	if (url[0] == 0)
		return output_one_page(sock, ws, " ", 1, "");

	for (int i = 0; m_url_func[i].url; i++) {
		struct url_func *p = &m_url_func[i];
		if (strcmp(p->url, url) == 0) {
			return (*p->func)(ws, http, sock, pthread_idx);
		}
	}

	if (check_digit(url))
		return my_find_index(ws, http, sock, pthread_idx);

error:
	ws->ws_http_output_head(sock, 404, "text/html", 10);
	ws->ws_http_output_body(sock, "Not Found\n", 10);

	return 0;
}

int _check_data_file(int idx)
{
	return m_wiki_data->wd_check_fd(idx);
}

/*
 * a part copy from wiki.cpp
 */
int wiki_init()
{
	for (int i = 0; i < MAX_PTHREAD_TOTAL; i++) {
		m_math_data[i] = (char *)malloc(512*1024);
		m_buf_data[i] = (char *)malloc(MAX_PAGE_LEN);
		m_tmp_data[i] = (char *)malloc(MAX_PAGE_LEN);
	}

	m_wiki_config = new WikiConfig();
	m_wiki_config->wc_init(".");

	char default_lang[64];
	struct file_st *f;
	int total;

	m_wiki_config->wc_get_lang_list(default_lang, &f, &total);

	m_file = &f[0];

	if (total == 0) {
		LOG("Not found any data file ... ?\n");
		return -1;
	}

	m_wiki_data = new WikiData();
	if (m_wiki_data->wd_init(m_file->data_file, m_file->data_total) == -1) {
		LOG("Not found\n");
		return -1;
	}
	m_wiki_data->wd_get_head(&m_data_head);

	m_wiki_index = new WikiIndex();
	if (m_wiki_index->wi_init(m_file->index_file) == -1) {
		LOG("Not found %s: %s\n", m_file->index_file, strerror(errno));
		return -1;
	}

	m_wiki_index->wi_set_data_func(_check_data_file);

#ifdef WIN32
	m_wiki_math = new WikiMath();
	m_wiki_math->wm_init(m_file->math_file);

	m_wiki_image = new WikiImage();
	m_wiki_image->we_init(m_file->image_file, m_file->image_total);
#endif

	m_wiki_full_text = new WikiFullIndex();
	m_wiki_full_text->wfi_init(m_file->fidx_file, m_file->fidx_file_total);
	m_wiki_full_text->wfi_stat();

	m_wiki_socket = new WikiSocket();
	m_wiki_socket->ws_init(my_wait, my_do_url, NULL);
#ifndef WIN32
	m_wiki_socket->ws_start_http_server(-MAX_PTHREAD_TOTAL, false);
#else
	m_wiki_socket->ws_start_http_server(MAX_PTHREAD_TOTAL, false);
#endif

	return 0;
}

int main(int argc, char *argv[])
{
	printf("---====== Fastwiki on PC ======---\n");
	printf("Author: qianshanhai\n");
	printf("Version: %s, %s %s\n", _VERSION, __DATE__, __TIME__);

	wiki_init();

	fgetc(stdin);

	return 0;
}

/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "q_util.h"
#include "q_log.h"

#include "wiki_data.h"
#include "wiki_index.h"
#include "wiki_full_index.h"

#include "wiki_local.h"
#include "wiki_socket.h"
#include "wiki_math.h"
#include "wiki_image.h"
#include "wiki_scan_file.h"

#include "httpd-index.h"
#include "fastwiki-httpd.h"

#define MAX_PTHREAD_TOTAL 10
#define MAX_PAGE_LEN (5*1024*1024)

char *m_math_data[MAX_PTHREAD_TOTAL];
char *m_convert_data[MAX_PTHREAD_TOTAL];
char *m_buf_data[MAX_PTHREAD_TOTAL];
char *m_tmp_data[MAX_PTHREAD_TOTAL];

static WikiIndex *m_wiki_index = NULL;
static WikiData *m_wiki_data = NULL;
static WikiMath *m_wiki_math = NULL;
static WikiImage *m_wiki_image = NULL;
static WikiFullIndex *m_wiki_full_text = NULL;

static WikiSocket *m_wiki_socket = NULL;
static HttpdIndex *m_httpd_index = NULL;

static struct file_st *m_file;
static data_head_t m_data_head;

pthread_mutex_t m_mutext;

#define _is_http_full_text(_h) (_h->hp_cookie("fulltext")[0] == '1' || _h->hp_param("ft")[0] != 0)

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

static int output_one_page(int sock, WikiSocket *ws, HttpParse *http, const char *data, int n, const char *K)
{
	char start_html[8192];
	char end_html[2048];
	int start_len, end_len;
	int size;
	
	start_len = sprintf(start_html, HTTP_START_HTML, _is_http_full_text(http) ? "checked" : "",
				http->hp_cookie("showall")[0] == '1' ? "checked" : "");

	end_len = sprintf(end_html, HTTP_END_HTML, K);

	size = start_len + end_len + n;

	ws->ws_http_output_head(sock, 200, "text/html", size);
	ws->ws_http_output_body(sock, start_html, start_len);
	ws->ws_http_output_body(sock, data, n);
	ws->ws_http_output_body(sock, end_html, end_len);

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

#define _FW_URL_FUNC_ARGV WikiSocket *ws, HttpParse *http, int sock, int pthread_idx

#define _TMP_DATA_START (4*1024)

static int my_find_full_text(_FW_URL_FUNC_ARGV, const char *key)
{
	int n, total, all_total;
	char *buf_data = m_buf_data[pthread_idx];
	char *tmp_data = m_tmp_data[pthread_idx];
	char title[128], message[_TMP_DATA_START];
	int page_idx[16], message_pos = 0;
	int page = atoi(http->hp_param("page"));
	struct timeval diff1, diff2;

	gettimeofday(&diff1, NULL);

	pthread_mutex_lock(&m_mutext);

	total = m_httpd_index->hi_fetch(key, &page, page_idx, &all_total);

	pthread_mutex_unlock(&m_mutext);

	if (total <= 0) {
		n = sprintf(buf_data, "Not found: %s\n", key);
		return output_one_page(sock, ws, http, buf_data, n, key);
	}

	int pos = _TMP_DATA_START, len;
	sort_idx_t idx;

	pos += sprintf(tmp_data + pos, "<table border=0>\n");

	for (int i = 0; i < total; i++) {
		if (m_wiki_index->wi_find(NULL, WK_INDEX_FIND, page_idx[i], &idx, &len) == -1)
			continue;

		m_wiki_index->wi_get_key(&idx, title);
		if ((len = m_wiki_data->wd_sys_read(idx.data_file_idx, idx.data_pos, idx.data_len, buf_data, MAX_PAGE_LEN)) <=  0)
			continue;

		buf_data[len] = 0;

		char *p = buf_data;

		if (http->hp_cookie("showall")[0] != '1') {
			p = m_convert_data[pthread_idx];
			len = convert_page_complex(p, buf_data, len, key);
		} else {
			p = m_convert_data[pthread_idx];
			len = convert_page_simple(p, buf_data, len, key);
		}

		pos += sprintf(tmp_data + pos, "<tr><td width=600px>"
					"<a href='%d?title=%s&key=%s'><font size=4>%s</font></a><br/>\n",
					page_idx[i], title, key, title);

		memcpy(tmp_data + pos, p, len);
		pos += len;

		pos += sprintf(tmp_data + pos, "%s", "<br/><br/>\n</td</tr>\n");
	}

	pos += sprintf(tmp_data + pos, "</table>\n");

	gettimeofday(&diff2, NULL);
	if (diff2.tv_usec < diff1.tv_usec) {
		diff2.tv_usec += 1000000;
		diff2.tv_sec--;
	}

	message_pos += sprintf(message + message_pos,
			"<table border=0><tr><td width=600px>Total: %d, Time: %d.%05ds <br/><br/></td></tr></table>\n",
			all_total, (int)(diff2.tv_sec - diff1.tv_sec), (int)(diff2.tv_usec - diff1.tv_usec) / 10);

	int page_total = _page_total(all_total);
	int before_page = page - 1;
	int next_page = page + 1;

	if (before_page <= 0)
		before_page = 1;

	if (next_page >= page_total)
		next_page = page_total;

	pos += sprintf(tmp_data + pos, "<table border=0 width=100%%><tr><td align=left width=40%%>"
			"<a href=\"/search?key=%s&ft=on&page=%d\">"
			"<font size=4><b>&lt;&lt;&lt;&lt;&lt;</b></font></a></td>\n", key, before_page);

	pos += sprintf(tmp_data + pos, "<td align=center width=20%%>%d/%d</td>", page, page_total);

	pos += sprintf(tmp_data + pos, "<td align=right width=40%%>"
			"<a href=\"/search?key=%s&ft=on&page=%d\">"
			"<font size=4><b>&gt;&gt;&gt;&gt;&gt;</b></font></a></td></tr></table>\n<br/>", key, next_page);

	char *tmp = tmp_data + _TMP_DATA_START - message_pos;
	memcpy(tmp, message, message_pos);

	output_one_page(sock, ws, http, tmp, pos + message_pos - _TMP_DATA_START, http->hp_param("key"));

	return 0;
}

static int my_find_key(_FW_URL_FUNC_ARGV)
{
	int total, n;
	char key[256];
	sort_idx_t idx;
	char *buf_data = m_buf_data[pthread_idx];
	char *tmp_data = m_tmp_data[pthread_idx];

	strncpy(key, http->hp_param("key"), sizeof(key) - 1);
	key[sizeof(key) - 1] = 0;

	trim(key); /* TODO */
	q_tolower(key);

	if (_is_http_full_text(http))
		return my_find_full_text(ws, http, sock, pthread_idx, key);

	if (m_wiki_index->wi_find(key, WK_KEY_FIND, -1, &idx, &total) == 0) {
		if ((n = m_wiki_data->wd_sys_read((int)idx.data_file_idx, idx.data_pos,
						(int)idx.data_len, buf_data, MAX_PAGE_LEN)) > 0) {
			char *data = convert_nohtml(buf_data, n, tmp_data, &n);
			output_one_page(sock, ws, http, data, n, http->hp_param("key"));
			return 0;
		}
	}

	return output_one_page(sock, ws, http, "Not Found", 9, "");
}

#include "logo-png.h"

static int my_output_logo_png(_FW_URL_FUNC_ARGV)
{
	int len = sizeof(_HTTPD_LOGO_PNG) - 1;

	ws->ws_http_output_head(sock, 200, "image/png", len);
	ws->ws_http_output_body(sock, _HTTPD_LOGO_PNG, len);

	return 0;
}

static int my_find_image(_FW_URL_FUNC_ARGV, const char *file)
{
	int size, one_block;
	char *buf_data = m_buf_data[pthread_idx];

	char tmp[1024];

	http->hp_www_decode(file, tmp, sizeof(tmp));

#ifndef WIN32
	if (m_wiki_image == NULL) {
		m_wiki_image = new WikiImage();
		m_wiki_image->we_init(m_file->image_file, m_file->image_total);
	}
#endif

	if (m_wiki_image->we_reset(pthread_idx, tmp, &size) == 0) {
		if (strncasecmp(file + strlen(file) - 4, ".svg", 4) == 0)
			ws->ws_http_output_head(sock, 200, "image/svg+xml", size);
		else
			ws->ws_http_output_head(sock, 200, "image/png", size);

		while (m_wiki_image->we_read_next(pthread_idx, buf_data, &one_block)) {
			ws->ws_http_output_body(sock, buf_data, one_block);
		}
		return 0;
	}

	return output_one_page(sock, ws, http, "Not Found", 9, "");
}

static int my_find_math(_FW_URL_FUNC_ARGV, const char *file)
{
	int len;
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

	return output_one_page(sock, ws, http, "Not Found", 9, "");
}

static int my_find_match(_FW_URL_FUNC_ARGV)
{
	sort_idx_t idx[MAX_FIND_RECORD];
	char *key = http->hp_param("key");

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
			wiki_convert_key(m_wiki_index, key, title, (int)x->key_len, tmp);
			len += snprintf(buf + len, sizeof(buf) - len,
					"<a href='search?key=%s'>%s</a><br/>", title, tmp);
		} else {
			len += snprintf(buf + len, sizeof(buf) - len,
					"<a href='search?key=%s'>%s</a><br/>", title, title);
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
	char *key = http->hp_param("key");

	if ((n = m_wiki_data->wd_sys_read(file_idx, (unsigned int)pos, len, buf_data, MAX_PAGE_LEN)) > 0) {
		char *w = convert_nohtml(buf_data, n, tmp_data, &n);
		return output_one_page(sock, ws, http, w, n, key);
	}

	return output_one_page(sock, ws, http, "Not Found", 9, key);
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
	char *convert = m_convert_data[pthread_idx];

	split(',', url, sp);

	for (int i = 0; i < split_total(sp); i++) {
		if (m_wiki_index->wi_find(NULL, WK_INDEX_FIND, atoi(sp[i]), &idx, &total) == 0) {
			if ((n = m_wiki_data->wd_sys_read((int)idx.data_file_idx, idx.data_pos,
							(int)idx.data_len, buf_data, MAX_PAGE_LEN)) > 0) {
				m_wiki_index->wi_get_key(&idx, key);
				char *w = convert_nohtml(buf_data, n, tmp_data, &n);
				if (http->hp_param("key")[0]) {
					n = convert_page_simple(convert, w, n, http->hp_param("key"));
					return output_one_page(sock, ws, http, convert, n, key);
				}
				output_one_page(sock, ws, http, w, n, key);
				return 0;
			}
		}
	}

	return output_one_page(sock, ws, http, "Not Found", 9, "");
}

struct url_func {
	const char *url;
	int (*func)(_FW_URL_FUNC_ARGV);
};

struct url_func m_url_func[] = {
	{"search", my_find_key},
	{"match", my_find_match},
	{"logo.png", my_output_logo_png},
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

	url = http->hp_url();

#ifdef DEBUG
	LOG("url=%s.\n", url);
#endif

	if (url[0] == 0)
		return output_one_page(sock, ws, http, " ", 1, "");

	if (strncmp(url, "I.", 2) == 0)
		return my_find_image(ws, http, sock, pthread_idx, url + 2);

	if (url[0] == 'M')
		return my_find_math(ws, http, sock, pthread_idx, url);

	if (url[0] >= '0' && url[0] <= '9')
		return my_find_index(ws, http, sock, pthread_idx);

	for (int i = 0; m_url_func[i].url; i++) {
		struct url_func *p = &m_url_func[i];
		if (strcmp(p->url, url) == 0) {
			return (*p->func)(ws, http, sock, pthread_idx);
		}
	}

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
int wiki_init(const char *dir)
{
	pthread_mutex_init(&m_mutext, NULL);

	for (int i = 0; i < MAX_PTHREAD_TOTAL; i++) {
		m_math_data[i] = (char *)malloc(512*1024);
		m_buf_data[i] = (char *)malloc(MAX_PAGE_LEN);
		m_tmp_data[i] = (char *)malloc(MAX_PAGE_LEN);
		m_convert_data[i] = (char *)malloc(MAX_PAGE_LEN);
	}

	fw_dir_t tmp;
	int lang_total;
	struct file_st file[4];

	strcpy(tmp.path, dir);

	WikiScanFile *wsf = new WikiScanFile();
	lang_total = wsf->wsf_scan_file(file, 4, &tmp, 1);

	if (lang_total == 0) {
		LOG("Not found any data file in folder %s ?\n", dir);
		return -1;
	}

	m_file = &file[0];

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

#ifdef DEBUG
	m_wiki_full_text->wfi_stat();
#endif

	m_httpd_index = new HttpdIndex();
	m_httpd_index->hi_init(m_wiki_full_text);

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
	const char *dir = ".";

	if (argc > 1)
		dir = argv[1];

	printf("---====== Fastwiki on PC ======---\n");
	printf("Author: qianshanhai\n");
	printf("Version: %s, %s %s\n", _VERSION, __DATE__, __TIME__);

	wiki_init(dir);

	fgetc(stdin);

	return 0;
}

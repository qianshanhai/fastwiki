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

#include "wiki_config.h"
#include "wiki_local.h"
#include "wiki_socket.h"
#include "wiki_math.h"
#include "wiki_image.h"

#include "fastwiki-httpd.h"

#define MAX_PTHREAD_TOTAL 10
#define MAX_PAGE_LEN (5*1024*1024)

char *m_math_data[MAX_PTHREAD_TOTAL];
char *m_buf_data = NULL;
char *m_tmp_data = NULL;

char start_html[4096];
char end_html[4096];
int start_html_len = 0;
int end_html_len = 0;

static WikiConfig *m_wiki_config = NULL;

static WikiIndex *m_wiki_index = NULL;
static WikiData *m_wiki_data = NULL;
static WikiSocket *m_wiki_socket = NULL;
static WikiMath *m_wiki_math = NULL;
static WikiImage *m_wiki_image = NULL;

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

static int my_do_url(void *_class, void *type, int sock, const char *_url, int idx)
{
	int len;
	char url[1024], *p;
	char *math_data = m_math_data[idx];
	char *buf_data = m_buf_data;
	WikiSocket *ws = (WikiSocket *)type;

	start_html_len = sprintf(start_html, HTTP_START_HTML, ws->ws_get_host());
	end_html_len = sprintf(end_html, "%s", HTTP_END_HTML);

	strncpy(url, _url, sizeof(url) - 1);

#ifdef DEBUG
	LOG("url=%s.\n", url);
#endif

	if (url[0] == 0 || strcmp(url, "/") == 0 || strncasecmp(url, "HTTP", 4) == 0) {
		return output_one_page(sock, ws, "", 0, "");
	}

	if ((p = strstr(url, "K="))) {
		p += 2;
		if (*p == 0)
			goto error;

		url_convert(p);

		sort_idx_t idx;
		int total, n;
		if (m_wiki_index->wi_find(p, WK_KEY_FIND, -1, &idx, &total) == 0) {
			if ((n = m_wiki_data->wd_sys_read((int)idx.data_file_idx, idx.data_pos,
							(int)idx.data_len, buf_data, MAX_PAGE_LEN)) > 0) {
				char *w = convert_nohtml(buf_data, n, m_tmp_data, &n);
				output_one_page(sock, ws, w, n, p);
				return 0;
			}
		}
		return output_one_page(sock, ws, "Not Found", 9, "");
	}

	/* image */
	if (url[0] == 'I') {
		p = url + 2;
		url_convert(p);

		int size, one_block;

#ifndef WIN32
		if (m_wiki_image == NULL) {
			m_wiki_image = new WikiImage();
			m_wiki_image->we_init(m_file->image_file, m_file->image_total);
		}
#endif
		if (m_wiki_image->we_reset(idx, p, &size) == 0) {
			if (strncasecmp(url + strlen(url) - 4, ".svg", 4) == 0)
				ws->ws_http_output_head(sock, 200, "image/svg+xml", size);
			else
				ws->ws_http_output_head(sock, 200, "image/png", size);

			while (m_wiki_image->we_read_next(idx, math_data, &one_block)) {
				ws->ws_http_output_body(sock, math_data, one_block);
			}
			return 0;
		}
		goto error;
	}

	/* math */
	if (url[0] == 'M') {
#ifndef WIN32
		if (m_wiki_math == NULL) {
			m_wiki_math = new WikiMath();
			m_wiki_math->wm_init(m_file->math_file);
		}
#endif
		if (m_wiki_math->wm_find(url, math_data, &len, 0)) {
			ws->ws_http_output_head(sock, 200, "image/png", len);
			ws->ws_http_output_body(sock, math_data, len);
			return 0;
		}
		goto error;
	}

	if (url[0] == 'F') {
		sort_idx_t *idx = (sort_idx_t *)calloc(sizeof(sort_idx_t), 128);
		int total;
		int match_flag = 0;

		p = url + 2;
		url_convert(p);

		if (m_wiki_index->wi_find(p, WK_LIKE_FIND, -1, idx, &total) == -1) {
			match_flag = 1;
			if (m_wiki_index->wi_find(p, WK_MATCH_FIND, -1, idx, &total) == -1) {
				free(idx);
				char buf[] = "Not Found\n";
				len = strlen(buf);
				//output_one_page(sock, ws, buf, len, p);
				ws->ws_http_output_head(sock, 200, "text/html", len);
				ws->ws_http_output_body(sock, buf, len);
				return 0;
			}
		}

		char key[1024];
		char buf[4096];
		char tmp[2048];

		len = 0;

		for (int i = 0; i < total && i < 25; i++) {
			sort_idx_t *x = &idx[i];
			m_wiki_index->wi_get_key(x, key);
			if (match_flag) {
				wiki_conv_key(p, key, (int)x->key_len, tmp);
				len += snprintf(buf + len, sizeof(buf) - len,
						"<a href='#%s' onclick=\"get('P:%d,%d,%d');\">%s</a><br/>",
						key, x->data_file_idx, x->data_pos, x->data_len, tmp);
			} else {
				len += snprintf(buf + len, sizeof(buf) - len,
						"<a href='#%s' onclick=\"get('P:%d,%d,%d');\">%s</a><br/>",
						key, x->data_file_idx, x->data_pos, x->data_len, key);
			}
		}

		ws->ws_http_output_head(sock, 200, "text/html", len);
		ws->ws_http_output_body(sock, buf, len);
		free(idx);
		return 0;
	}

	if (url[0] == 'P') {
		split_t sp;
		int n;
		split(',', url + 2, sp);
		if (split_total(sp) != 3)
			goto error;

		if ((n = m_wiki_data->wd_sys_read(atoi(sp[0]), (unsigned int)atoi(sp[1]), atoi(sp[2]),
						buf_data, MAX_PAGE_LEN)) > 0) {
			char *w = convert_nohtml(buf_data, n, m_tmp_data, &n);
			ws->ws_http_output_head(sock, 200, "text/html", n);
			ws->ws_http_output_body(sock, w, n);
			return 0;
		}
	}
	
	if (check_digit(url)) {
		split_t sp;
		split(',', url, sp);
		sort_idx_t idx;
		int total, n;
		char key[1024] = {0};
		for (int i = 0; i < split_total(sp); i++) {
			if (m_wiki_index->wi_find(NULL, WK_INDEX_FIND, atoi(sp[i]), &idx, &total) == 0) {
				if ((n = m_wiki_data->wd_sys_read((int)idx.data_file_idx, idx.data_pos,
								(int)idx.data_len, buf_data, MAX_PAGE_LEN)) > 0) {
					m_wiki_index->wi_get_key(&idx, key);
					char *w = convert_nohtml(buf_data, n, m_tmp_data, &n);
					output_one_page(sock, ws, w, n, key);
					return 0;
				}
			}
		}
		return output_one_page(sock, ws, "Not Found", 9, "");
	}

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
	}

	m_buf_data = (char *)malloc(MAX_PAGE_LEN);
	m_tmp_data = (char *)malloc(MAX_PAGE_LEN);

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
	printf("Version: %s\n", _VERSION);
	printf("Date: %s\n", __DATE__);

	wiki_init();

	fgetc(stdin);

	return 0;
}

/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <stdlib.h>

#include "q_util.h"
#include "q_socket.h"

#include "http_parse.h"

HttpParse::HttpParse()
{
	memset(m_host, 0, sizeof(m_host));
	memset(m_url, 0, sizeof(m_url));
}

HttpParse::~HttpParse()
{
}

int HttpParse::hp_init(int sock)
{
	int len;
	char buf[4096];

	buf[0] = 0;

	if ((len = q_read_data(sock, buf, sizeof(buf) - 1, 10*1000000)) > 0)
		buf[len] = 0;

	return hp_init_all(buf, len);
}

int HttpParse::hp_init_all(char *buf, int len)
{
	int content_len = 0;
	char *p, *w, *remain = NULL, *content = NULL;

	m_host[0] = 0;
	m_url[0] = 0;
	m_cookie_total = 0;
	m_param_total = 0;

	if ((remain = strcasestr(buf, "HTTP")) == NULL)
		remain = strchr(buf, '\n');

	if (remain == NULL) {
		remain = (char *)"";
	} else {
		*remain++ = 0;
	}

	if (strncmp(buf, "GET ", 4) == 0 || strncmp(buf, "POST ", 5) == 0) {
		for (p = buf + 4; *p == ' ' || *p == '\t' || *p == '/'; p++);
		if ((w = strchr(p, '?'))) {
			*w++ = 0;
			content = w;
		} else {
			if ((w = strchr(p, ' ')))
				*w = 0;
		}
		strncpy(m_url, p, sizeof(m_url) - 1);
	}

	char *host = NULL, *cookie = NULL;

	for (w = remain; *w; w++) {	
		if (*w == '\n') {
			if (strncasecmp(w + 1, "Host:", 5) == 0) {
				host = w + 6;
			} else if (strncasecmp(w + 1, "Cookie:", 7) == 0) {
				cookie = w + 8;
			} else if (strncasecmp(w + 1, "Content-Length:", 15) == 0) {
				content_len = atoi(w + 16);
			} else if (w[1] == '\r' && w[2] == '\n') {
				if (content == NULL)
					content = w + 3;
			}
		}
	}

	if (host)
		hp_init_host(host);

	if (cookie)
		hp_init_cookie(cookie);

	if (content != NULL)
		hp_init_param(content);

	return 0;
}

int HttpParse::hp_init_host(char *p)
{
	char *w;

	for (; *p == ' ' || *p == '\t'; p++);
	if ((w = strchr(p, '\r')) != NULL || (w = strchr(p, '\n')) != NULL)
		*w = 0;

	strncpy(m_host, p, sizeof(m_host) - 1);

	return 0;
}

int HttpParse::hp_init_cookie(char *p)
{
	char *w;
	split_t sp;

	m_cookie_total = 0;
	memset(m_cookie, 0, sizeof(m_cookie));

	if ((w = strchr(p, '\r')) == NULL || (w = strchr(p, '\n')) == NULL)
		return 0;

	*w = 0;

	split(';', p, sp);

	for_each_split(sp, buf) {
		if ((w = strchr(buf, '=')) == NULL)
			continue;
		*w++ = 0;

		struct http_param *k = &m_cookie[m_cookie_total++];
		
		strncpy(k->name, trim(buf), sizeof(k->name) - 1);
		strncpy(k->value, trim(w), sizeof(k->value) - 1);
	}

	return m_cookie_total;
}

#define _HP_CHECK_TMP_LEN(_c) \
	if (pos >= len - 1) \
		break; \
	tmp[pos++] = _c

char *HttpParse::hp_www_decode(const char *buf, char *tmp, int len)
{
	char ch;
	int pos = 0;

	for (int i = 0; buf[i]; i++) {
		ch = buf[i];
		if (ch == '+') {
			_HP_CHECK_TMP_LEN(' ');
		} else if (ch == '%') {
			if ((ch = (char)q_hex2ch(buf + i + 1)) == 0)
				break;
			_HP_CHECK_TMP_LEN(ch);
			i += 2;
		} else {
			_HP_CHECK_TMP_LEN(ch);
		}
	}

	tmp[pos] = 0;

	return tmp;
}

int HttpParse::hp_init_param(char *p)
{
	split_t sp;
	char *w, tmp[1024];
	struct http_param *k;

	m_param_total = 0;

	split('&', p, sp);

	for_each_split(sp, buf) {
		if ((w = strchr(buf, '='))) {
			*w++ = 0;
			k = &m_param[m_param_total++];

			strncpy(k->name, hp_www_decode(buf, tmp, sizeof(tmp)), sizeof(k->name) - 1);
			strncpy(k->value, hp_www_decode(w, tmp, sizeof(tmp)), sizeof(k->value) - 1);
		}
	}

	return m_param_total;
}

char *HttpParse::hp_fetch(struct http_param *buf, int total, const char *name)
{
	struct http_param *p;

	for (int i = 0; i < total; i++) {
		p = &buf[i];
		if (strcmp(p->name, name) == 0)
			return p->value;
	}

	return (char *)"";
}

char *HttpParse::hp_param(const char *name)
{
	return hp_fetch(m_param, m_param_total, name);
}

char *HttpParse::hp_cookie(const char *name)
{
	return hp_fetch(m_cookie, m_cookie_total, name);
}

void HttpParse::hp_print_param(struct http_param *buf, int total)
{
	struct http_param *p;

	for (int i = 0; i < total; i++) {
		p = &buf[i];
		printf(" [%s = %s]\n", p->name, p->value);
	}
}

void HttpParse::hp_stat()
{
	printf("m_host: %s\n", m_host);
	printf("m_url: %s\n", m_url);

	printf("cookie: \n");
	hp_print_param(m_cookie, m_cookie_total);

	printf("param: \n");
	hp_print_param(m_param, m_param_total);
}

const char *HttpParse::hp_host()
{
	return m_host;
}

const char *HttpParse::hp_url()
{
	return m_url;
}
